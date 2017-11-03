#include "PCH/stdafx.h"
#include "SceneLoader.h"

#include <pugixml.hpp>

namespace GlassEngine
{

	bool SceneLoader::LoadSceneAssets(const char* sceneName)
	{
		// Ask pugi to load the .glassScene XML file
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file(sceneName);

		textureFilePaths.clear();
		modelFilePaths.clear();
		audioFilePaths.clear();

		if (result)
		{
			auto root = doc.root().child("Scene");
			for (pugi::xml_node model = root.child("Models").child("Model"); model; model = model.next_sibling("Model"))
			{
				std::string str = model.attribute("path").as_string();
				modelFilePaths.push_back(str);
			}

			for (pugi::xml_node texture = root.child("Textures").child("Texture"); texture; texture = texture.next_sibling("Texture"))
			{
				std::string str = texture.attribute("path").as_string();
				textureFilePaths.push_back(str);
			}

			for (pugi::xml_node audioClip = root.child("AudioFiles").child("AudioClip"); audioClip; audioClip = audioClip.next_sibling("AudioClip"))
			{
				std::string str = audioClip.attribute("path").as_string();
				audioFilePaths.push_back(str);
			}

			return true;
		}
		
		// If pugi wasn't able to load the scene file then throw an error to the developer / user
		std::cout << "XML [" << sceneName << "] parsed with errors, attr value: [" << doc.child("node").attribute("attr").value() << "]\n";
		std::cout << "Error description: " << result.description() << "\n";
		std::cout << "Error offset: " << result.offset << " (error at [..." << (sceneName + result.offset) << "]\n\n";
		return false;
	}

	size_t SceneLoader::GetTextureCount() const
	{
		return textureFilePaths.size();
	}

	size_t SceneLoader::GetModelCount() const
	{
		return modelFilePaths.size();
	}

	size_t SceneLoader::GetAudioCount() const
	{
		return audioFilePaths.size();
	}


	const char* SceneLoader::GetAudioFile(int idx)
	{
		if (idx >= audioFilePaths.size()) return nullptr;
		return audioFilePaths[idx].c_str();
	}

	const char* SceneLoader::GetModelFile(int idx)
	{
		if (idx >= modelFilePaths.size()) return nullptr;
		return modelFilePaths[idx].c_str();
	}

	const char* SceneLoader::GetTextureFile(int idx)
	{
		if (idx >= textureFilePaths.size()) return nullptr;
		return textureFilePaths[idx].c_str();
	}
}
