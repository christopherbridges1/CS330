///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	// loaded the textures for the 3D scene, Added references to each texture
	CreateGLTexture("textures/wood.jpg", "woodTexture");			//https://commons.wikimedia.org/wiki/File:Balsa_Wood_Texture.jpg
	CreateGLTexture("textures/leather.jpg", "leatherTexture");		//https://commons.wikimedia.org/wiki/File:Black_Leather.jpg
	CreateGLTexture("textures/cube.jpg", "cubeTexture");			//Made it myself in paint... not an artist
	CreateGLTexture("textures/can.jpg", "canTexture");				
	CreateGLTexture("textures/top.png", "topTexture");			
	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	// definition for the materials for the objects in the scene
	// 
	// wood material
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	woodMaterial.ambientStrength = 0.3f;
	woodMaterial.diffuseColor = glm::vec3(0.6f, 0.4f, 0.2f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 8.0f;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	// leather material
	OBJECT_MATERIAL leatherMaterial;
	leatherMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	leatherMaterial.ambientStrength = 0.2f;
	leatherMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	leatherMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	leatherMaterial.shininess = 0.5;
	leatherMaterial.tag = "leather";
	m_objectMaterials.push_back(leatherMaterial);

	// metal material
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	metalMaterial.ambientStrength = 0.3f;
	metalMaterial.diffuseColor = glm::vec3(0.2f, 0.6f, 0.4f);
	metalMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	metalMaterial.shininess = 13.0;
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);

	// plastic material
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	plasticMaterial.ambientStrength = 0.2f;
	plasticMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	plasticMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	plasticMaterial.shininess = 0.5;
	plasticMaterial.tag = "plastic";
	m_objectMaterials.push_back(plasticMaterial);
}
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	//m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/

// ***** Cool Blue from Above *******
	m_pShaderManager->setVec3Value("lightSources[0].position", 0.0f, 10.0f, 0.0f); 
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.1f, 0.1f, 0.2f); 
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.6f, 0.7f, 1.0f); 
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.4f, 0.4f, 1.0f); 
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 0.5f); 
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.4f); 

	// ****** Warm Side Glow *****
	m_pShaderManager->setVec3Value("lightSources[1].position", -7.0f, 4.0f, 2.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.02f, 0.015f, 0.01f); 
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.8f, 0.4f, 0.1f); 
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.6f, 0.3f, 0.2f); 
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 0.2f); 
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.3f); 

	// ***** Rim Light ******
	m_pShaderManager->setVec3Value("lightSources[2].position", 8.0f, -3.0f, 10.0f); 
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.02f, 0.02f, 0.05f); 
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.2f, 0.3f, 0.7f); 
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.2f, 0.2f, 0.8f); 
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 0.6f); 
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.4f); 

	// ***** Top Front Light) *****
	m_pShaderManager->setVec3Value("lightSources[3].position", 12.0f, 6.0f, 10.0f); 
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.03f, 0.03f, 0.03f); 
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.9f, 0.9f, 0.9f); 
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 1.0f, 1.0f, 1.0f); 
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 0.2f); 
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.2f); 

	// ***** Enables shader lighting *****
	m_pShaderManager->setBoolValue("bUseLighting", true);


}
/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D sceneaaa
	LoadSceneTextures();
	SetupSceneLights();
	DefineObjectMaterials();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	/* Added necessary shapes to be loaded */
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadBoxMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -0.1f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color for the mesh
	SetShaderColor(0.55f, 0.27f, 0.07f, 1.0f);

	// set the texture data into the shader
	SetShaderTexture("woodTexture");
	SetShaderMaterial("wood");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();


	/******** Black Desk mat ********/
	scaleXYZ = glm::vec3(10.0f, 1.0f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.1f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color for the mesh
	SetShaderColor(0.55f, 0.27f, 0.07f, 1.0f);

	// set the texture data into the shader
	SetShaderTexture("leatherTexture");
	SetShaderMaterial("leather");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/******** Red Desk mat Border********/
	scaleXYZ = glm::vec3(10.1f, 1.1f, 6.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color for the mesh
	SetShaderColor(1.0f, 0.1f, 0.0f, 1.0f);
	SetShaderMaterial("leather");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/******** Torus Stand Base ********/
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);
	positionXYZ = glm::vec3(0.0f, 0.25f, 0.0f);

	// Rotated on X axis to become a stand 
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Apply transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Dark Gray for the stand
	SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawTorusMesh();

	/******** Tapered Cylinder supporting top and bottom Tori ********/
	scaleXYZ = glm::vec3(0.05f, 0.5f, 0.4f);
	positionXYZ = glm::vec3(0.0f, 0.6f, 0.0f);

	// Apply transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Applied lighter gray to contrast other components
	SetShaderColor(0.3f, 0.3f, 0.3f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawCylinderMesh();

	/******** Pokeball base ********/
	scaleXYZ = glm::vec3(0.3f, 0.3f, 0.3f);
	positionXYZ = glm::vec3(0.0f, 1.0f, 0.0f);

	// Rotated on X axis to form a flat base
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Apply transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Applied gray for contrast
	SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawTorusMesh();

	/******** Red Pokeball Top Half ********/
	scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);
	positionXYZ = glm::vec3(0.0f, 2.0f, 0.0f);

	// Apply transformations
	SetTransformations(
		scaleXYZ,
		0.0f,
		0.0f,
		0.0f,
		positionXYZ);

	// Set Pokeball top half as Red
	SetShaderColor(1.0f, 0.0f, 0.0f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawHalfSphereMesh();

	/******** White Pokeball Bottom Half ********/
	scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);
	positionXYZ = glm::vec3(0.0f, 2.0f, 0.0f);

	// Apply transformations
	SetTransformations(
		scaleXYZ,
		180.0f,
		0.0f,
		0.0f,
		positionXYZ);

	// Set Pokeball bottom half as White
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawHalfSphereMesh();

	/******** Button on Pokeball ********/
	scaleXYZ = glm::vec3(0.15f, 0.15f, 0.15f);
	positionXYZ = glm::vec3(0.0f, 2.0f, 1.0f); // Centered on the ball 

	// Apply transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Applied white to button 
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawSphereMesh();

	/******** Band on Pokeball ********/
	scaleXYZ = glm::vec3(0.9f, 0.9f, 0.9f);
	positionXYZ = glm::vec3(0.0f, 2.0f, 0.0f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Apply transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Applied black to band 
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawTorusMesh();

	/******** Cube on the left of the Pokeball ********/
	scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);  
	positionXYZ = glm::vec3(-5.0f, 0.6f, 0.0f);  

	// Apply transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1f, 0.4f, 0.8f, 1.0f);  // Blue color for cube testing
	SetShaderTexture("cubeTexture");  // Apply texture to the cube
	SetShaderMaterial("plastic");  // Apply material to the cube
	m_basicMeshes->DrawBoxMesh();  // Render Cube


	/******** Can ********/
	/*** Can Body ***/
	scaleXYZ = glm::vec3(0.75f, 2.0f, 0.75f); // Tall and thin
	positionXYZ = glm::vec3(-3.0f, 0.1f, 0.0f); // Centered on the can

	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	SetTransformations(
		scaleXYZ, 
		XrotationDegrees, 
		YrotationDegrees, 
		ZrotationDegrees, 
		positionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("canTexture");
	SetShaderMaterial("metal"); 
	m_basicMeshes->DrawCylinderMesh();

	/*** Can Top ***/
	scaleXYZ = glm::vec3(0.76f, 0.04f, 0.76f);  // Very thin disc for lid 
	positionXYZ = glm::vec3(-3.0f, 2.11f, 0.0f); // Slightly above the body

	SetTransformations(
		scaleXYZ, 
		XrotationDegrees, 
		YrotationDegrees, 
		ZrotationDegrees, 
		positionXYZ);

	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);
	SetShaderTexture("topTexture");  
	SetShaderMaterial("metal");
	m_basicMeshes->DrawCylinderMesh();

	/*** Can Bottom (Disc) ***/
	scaleXYZ = glm::vec3(0.76f, 0.04f, 0.76f);  // Very thin disc for bottom
	positionXYZ = glm::vec3(-3.0f, 0.1f, 0.0f);
	
	SetTransformations(
		scaleXYZ, 
		XrotationDegrees, 
		YrotationDegrees, 
		ZrotationDegrees, 
		positionXYZ);
	
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);
	SetShaderMaterial("metal");
	m_basicMeshes->DrawCylinderMesh();
}