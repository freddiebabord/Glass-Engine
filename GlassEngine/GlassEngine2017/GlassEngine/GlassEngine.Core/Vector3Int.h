#pragma once

namespace GlassEngine
{

	class Vector3Int
	{
	public:
		Vector3Int() : x(0), y(0), z(0) { }
		Vector3Int(int x, int y, int z) :
			x(x), y(y), z(z) { }

		int x;
		int y;
		int z;
	};

}