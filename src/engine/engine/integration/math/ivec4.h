#pragma once

struct IVector4 : public XMINT4
{
	IVector4() noexcept : XMINT4(0, 0, 0, 0) {}
	constexpr explicit IVector4(const int32_t x) noexcept : XMINT4(x, x, x, x) {}
	constexpr IVector4(const int32_t x, const int32_t y, const int32_t z, const int32_t w) noexcept : XMINT4(x, y, z, w) {}
	IVector4(const IVector3& v, const int32_t w) noexcept : XMINT4(v.x, v.y, v.z, w) {}
	IVector4(const IVector2& v, const int32_t z, const int32_t w) noexcept : XMINT4(v.x, v.y, z, w) {}

	IVector4(const IVector4&) = default;
	IVector4& operator=(const IVector4&) = default;

	IVector4(IVector4&&) = default;
	IVector4& operator=(IVector4&&) = default;

	// Comparison operators
	bool operator == (const IVector4& v) const noexcept { return ((x == v.x) && (y == v.y) && (z == v.z) && (w == v.w)); }
	bool operator != (const IVector4& v) const noexcept { return ((x != v.x) || (y != v.y) || (z != v.z) || (w != v.w)); }

	// Assignment operators
	IVector4& operator+= (const IVector4& v) noexcept { x += v.x; y += v.y; z += v.z; z += v.z; w += v.w; return *this; }
	IVector4& operator-= (const IVector4& v) noexcept { x -= v.x; y -= v.y; z -= v.z; z -= v.z; w -= v.w; return *this; }
	IVector4& operator*= (const IVector4& v) noexcept { x *= v.x; y *= v.y; z *= v.z; z *= v.z; w *= v.w; return *this; }
	IVector4& operator+= (int32_t s) noexcept { x += s; y += s; z += s; w += s; return *this; }
	IVector4& operator*= (int32_t s) noexcept { x *= s; y *= s; z *= s; w *= s; return *this; }
	//IVector4& operator/= (int32_t s) noexcept { x /= s; y /= s; z /= s; w /= s; return *this; }

	// Unary operators
	IVector4 operator+ () const noexcept { return *this; }
	IVector4 operator- () const noexcept { return IVector4(-x, -y, -z, -w); }

	void Clamp(const IVector4& vmin, const IVector4& vmax) noexcept;
	void Clamp(const IVector4& vmin, const IVector4& vmax, IVector4& result) const noexcept;

	// Static functions
	static void Min(const IVector4& v1, const IVector4& v2, IVector4& result) noexcept;
	static IVector4 Min(const IVector4& v1, const IVector4& v2) noexcept;

	static void Max(const IVector4& v1, const IVector4& v2, IVector4& result) noexcept;
	static IVector4 Max(const IVector4& v1, const IVector4& v2) noexcept;

	static constexpr IVector4 zero() noexcept { return IVector4(0, 0, 0, 0); }
	static constexpr IVector4 one() noexcept { return IVector4(1, 1, 1, 1); }
	static constexpr IVector4 unitX() noexcept { return IVector4(1, 0, 0, 0); }
	static constexpr IVector4 unitY() noexcept { return IVector4(0, 1, 0, 0); }
	static constexpr IVector4 unitZ() noexcept { return IVector4(0, 0, 1, 0); }
};

FORCE_INLINE void IVector4::Clamp(const IVector4& vmin, const IVector4& vmax) noexcept
{
	using namespace DirectX;
	const XMVECTOR v1 = XMLoadSInt4(this);
	const XMVECTOR v2 = XMLoadSInt4(&vmin);
	const XMVECTOR v3 = XMLoadSInt4(&vmax);
	const XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreSInt4(this, X);
}

FORCE_INLINE void IVector4::Clamp(const IVector4& vmin, const IVector4& vmax, IVector4& result) const noexcept
{
	using namespace DirectX;
	const XMVECTOR v1 = XMLoadSInt4(this);
	const XMVECTOR v2 = XMLoadSInt4(&vmin);
	const XMVECTOR v3 = XMLoadSInt4(&vmax);
	const XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreSInt4(&result, X);
}

// Binary operators
FORCE_INLINE IVector4 operator+ (const IVector4& V1, const IVector4& V2) noexcept { return IVector4(V1.x + V2.x, V1.y + V2.y, V1.z + V2.z, V1.w + V2.w); }
FORCE_INLINE IVector4 operator- (const IVector4& V1, const IVector4& V2) noexcept { return IVector4(V1.x - V2.x, V1.y - V2.y, V1.z - V2.z, V1.w - V2.w); }
FORCE_INLINE IVector4 operator* (const IVector4& V1, const IVector4& V2) noexcept { return IVector4(V1.x * V2.x, V1.y * V2.y, V1.z * V2.z, V1.w * V2.w); }
FORCE_INLINE IVector4 operator* (const IVector4& v, int32_t s) noexcept { return IVector4(v.x * s, v.y * s, v.z * s, v.w * s); }
//FORCE_INLINE IVector4 operator/ (const IVector4& V1, const IVector4& V2) noexcept { return IVector4(V1.x / V2.x, V1.y / V2.y, V1.z / V2.z); }
//FORCE_INLINE IVector4 operator/ (const IVector4& v, float s) noexcept { return IVector4(v.x / s, v.y / s, v.z / s); }
FORCE_INLINE IVector4 operator* (int32_t s, const IVector4& v) noexcept { return IVector4(s * v.x, s * v.y, s * v.z, s * v.w); };
