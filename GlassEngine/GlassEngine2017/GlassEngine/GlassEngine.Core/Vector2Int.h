#pragma once

namespace GlassEngine
{
	class Vector2Int
	{
	public:
		Vector2Int() { }
		Vector2Int(int x, int y)
		{
			this->x = x;
			this->y = y;
		}

		union
		{
			int x;
			int width;
		};

		union
		{
			int y;
			int height;
		};
		
	};

}