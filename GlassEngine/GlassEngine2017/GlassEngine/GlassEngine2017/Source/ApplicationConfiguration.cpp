#include "PCH/stdafx.h"
#include "ApplicationConfiguration.h"

namespace GlassEngine
{
	ApplicationSettings *ApplicationSettings::instance = { nullptr };

	void ApplicationSettings::Init()
	{
		instance = this;
	}

	ApplicationSettings* ApplicationSettings::Instance()
	{
		return instance;
	}
}
