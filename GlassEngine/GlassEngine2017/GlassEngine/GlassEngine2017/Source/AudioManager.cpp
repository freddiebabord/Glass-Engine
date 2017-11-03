#include "PCH/stdafx.h"
#include "AudioManager.h"

namespace GlassEngine
{
	
	AudioSource AudioManager::LoadSound(const char* filename)
	{
		AudioSource source;
		if (!source.buffer.loadFromFile(filename))
		{
			printf("AUDIO :: ERROR :: Unable to load sound: %s", filename);
			std::ofstream file("sfml-log.txt");
			std::streambuf* previous = sf::err().rdbuf(file.rdbuf());
			abort();
		}
		source.sound.setBuffer(source.buffer);
		loadedSounds.insert(std::pair<const char*, AudioSource>(filename, source));
		return loadedSounds[filename];
	}

	void AudioManager::PlaySound(const char* name)
	{
		loadedSounds[name].sound.play();
	}
}
