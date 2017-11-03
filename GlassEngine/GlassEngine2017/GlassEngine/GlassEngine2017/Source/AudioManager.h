#pragma once

#include <SFML/Audio.hpp>

namespace GlassEngine
{
	
	struct AudioSource
	{
		sf::Sound sound;
		sf::SoundBuffer buffer;
	};

	class AudioManager
	{
	public:
		AudioSource LoadSound(const char* filename);
		void PlayStreamedAudio(const char* filename, bool loop);
		void PlaySound(const char* name);

	private:
		std::map<const char*, AudioSource> loadedSounds;
		std::vector<sf::Music> streamedAudioSources;
	};

}
