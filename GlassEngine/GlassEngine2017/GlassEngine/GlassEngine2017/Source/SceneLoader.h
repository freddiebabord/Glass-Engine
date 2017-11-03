#pragma once

namespace GlassEngine
{

	class SceneLoader
	{
	public:

		/// This doesn't actually load any assets. It's responsible for getting all the file names and definitions for the assets
		/// that will be used in the scene
		/// @param sceneName The scene file to load
		bool LoadSceneAssets(const char* sceneName);

		/// Get the amount of textures that are refrenced in the scene
		/// @return The amount of textures refrenced in the scene file
		size_t GetTextureCount() const;

		/// Get the amount of models that are refrenced in the scene
		/// @return The amount of models refrenced in the scene file
		size_t GetModelCount() const;

		/// Get the amount of audio files that are refrenced in the scene
		/// @return The amount of audio files refrenced in the scene file
		size_t GetAudioCount() const;

		/// Get a specific audio file in the scene
		/// @ idx The index of the audio file as it was defined in the scene definition
		/// @return The path of a specific audio file
		/// TODO: This needs to be changed into a map when an audio component is created
		const char* GetAudioFile(int idx);

		/// Get a specific model file in the scene
		/// @ idx The index of the model file as it was defined in the scene definition
		/// @return The path of a specific model file
		/// TODO: This needs to be changed into a map when a better scene renderer is created
		const char* GetModelFile(int idx);

		/// Get a specific texture file in the scene
		/// @ idx The index of the texture file as it was defined in the scene definition
		/// @return The path of a specific texture file
		/// TODO: This needs to be changed into a map when an audio component is created
		const char* GetTextureFile(int idx);

	private:
		std::vector<std::string> textureFilePaths, modelFilePaths, audioFilePaths;
		
	};

}