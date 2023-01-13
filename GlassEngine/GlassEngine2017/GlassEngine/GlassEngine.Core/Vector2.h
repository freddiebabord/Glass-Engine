#pragma once

namespace GlassEngine
{

	class Vector2
	{
	public:
		Vector2() : x(0), y(0) { }
		Vector2(float x, float y) :
			x(x), y(y) { }

		float x;
		float y;
	};

}