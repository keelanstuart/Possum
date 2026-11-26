#pragma once

template <class T> class TRange
{
protected:
	T v[2];

public:
	enum {RANGE_LOW = 0, RANGE_HIGH};

	TRange(T l, T h) { v[RANGE_LOW] = std::min<T>(l, h); v[RANGE_HIGH] = std::max<T>(l, h); }
	TRange(TRange &r) { v[RANGE_LOW] = r.v[RANGE_LOW]; v[RANGE_HIGH] = r.v[RANGE_HIGH]; }

	bool Intersects(const TRange &r) const
	{
		if (((r.v[RANGE_LOW] > v[RANGE_LOW]) && (r.v[RANGE_LOW] < v[RANGE_HIGH])) ||
			((v[RANGE_LOW] > r.v[RANGE_LOW]) && (v[RANGE_LOW] < r.v[RANGE_HIGH])) ||
			((r.v[RANGE_HIGH] > v[RANGE_LOW]) && (r.v[RANGE_HIGH] < v[RANGE_HIGH])) ||
			((v[RANGE_HIGH] > r.v[RANGE_LOW]) && (v[RANGE_HIGH] < r.v[RANGE_HIGH])))
			return true;

		return false;
	}
};

