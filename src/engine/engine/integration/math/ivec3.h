#pragma once

struct IVector3 : public XMINT3
{
	IVector3() noexcept : XMINT3(0, 0, 0) {}
	constexpr explicit IVector3(const int32_t x) noexcept : XMINT3(x, x, x) {}
	constexpr IVector3(const int32_t x, const int32_t y, const int32_t z) noexcept : XMINT3(x, y, z) {}
	IVector3(const IVector2& v, const int32_t z) noexcept : XMINT3(v.x, v.y, z) { }

	IVector3(const IVector3&) = default;
	IVector3& operator=(const IVector3&) = default;

	IVector3(IVector3&&) = default;
	IVector3& operator=(IVector3&&) = default;

	// Comparison operators
	bool operator == (const IVector3& v) const noexcept { return ((x == v.x) && (y == v.y) && (z == v.z)); }
	bool operator != (const IVector3& v) const noexcept { return ((x != v.x) || (y != v.y) || (z != v.z)); }

	// Assignment operators
	IVector3& operator+= (const IVector3& v) noexcept { x += v.x; y += v.y; z += v.z; return *this; }
	IVector3& operator-= (const IVector3& v) noexcept { x -= v.x; y -= v.y; z -= v.z; return *this; }
	IVector3& operator*= (const IVector3& v) noexcept { x *= v.x; y *= v.y; z *= v.z; return *this; }
	IVector3& operator+= (int32_t s) noexcept { x += s; y += s; z += s; return *this; }
	IVector3& operator*= (int32_t s) noexcept { x *= s; y *= s; z *= s; return *this; }
	//IVector3& operator/= (int32_t s) noexcept { x /= s; y /= s; z /= s; return *this; }

	// Unary operators
	IVector3 operator+ () const noexcept { return *this; }
	IVector3 operator- () const noexcept { return IVector3(-x, -y, -z); }

	void Clamp(const IVector3& vmin, const IVector3& vmax) noexcept;
	void Clamp(const IVector3& vmin, const IVector3& vmax, IVector3& result) const noexcept;

	// Static functions
	static void Min(const IVector3& v1, const IVector3& v2, IVector3& result) noexcept;
	static IVector3 Min(const IVector3& v1, const IVector3& v2) noexcept;

	static void Max(const IVector3& v1, const IVector3& v2, IVector3& result) noexcept;
	static IVector3 Max(const IVector3& v1, const IVector3& v2) noexcept;

	static constexpr IVector3 zero() noexcept { return IVector3(0, 0, 0); }
	static constexpr IVector3 one() noexcept { return IVector3(1, 1, 1); }
	static constexpr IVector3 unitX() noexcept { return IVector3(1, 0, 0); }
	static constexpr IVector3 unitY() noexcept { return IVector3(0, 1, 0); }
	static constexpr IVector3 unitZ() noexcept { return IVector3(0, 0, 1); }
};

FORCE_INLINE void IVector3::Clamp(const IVector3& vmin, const IVector3& vmax) noexcept
{
	using namespace DirectX;
	const XMVECTOR v1 = XMLoadSInt3(this);
	const XMVECTOR v2 = XMLoadSInt3(&vmin);
	const XMVECTOR v3 = XMLoadSInt3(&vmax);
	const XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreSInt3(this, X);
}

FORCE_INLINE void IVector3::Clamp(const IVector3& vmin, const IVector3& vmax, IVector3& result) const noexcept
{
	using namespace DirectX;
	const XMVECTOR v1 = XMLoadSInt3(this);
	const XMVECTOR v2 = XMLoadSInt3(&vmin);
	const XMVECTOR v3 = XMLoadSInt3(&vmax);
	const XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreSInt3(&result, X);
}

// Binary operators
FORCE_INLINE IVector3 operator+ (const IVector3& V1, const IVector3& V2) noexcept { return IVector3(V1.x + V2.x, V1.y + V2.y, V1.z + V2.z); }
FORCE_INLINE IVector3 operator- (const IVector3& V1, const IVector3& V2) noexcept { return IVector3(V1.x - V2.x, V1.y - V2.y, V1.z - V2.z); }
FORCE_INLINE IVector3 operator* (const IVector3& V1, const IVector3& V2) noexcept { return IVector3(V1.x * V2.x, V1.y * V2.y, V1.z * V2.z); }
FORCE_INLINE IVector3 operator* (const IVector3& v, int32_t s) noexcept { return IVector3(v.x * s, v.y * s, v.z * s); }
//FORCE_INLINE IVector3 operator/ (const IVector3& V1, const IVector3& V2) noexcept { return IVector3(V1.x / V2.x, V1.y / V2.y, V1.z / V2.z); }
//FORCE_INLINE IVector3 operator/ (const IVector3& v, float s) noexcept { return IVector3(v.x / s, v.y / s, v.z / s); }
FORCE_INLINE IVector3 operator* (int32_t s, const IVector3& v) noexcept { return IVector3(s * v.x, s * v.y, s * v.z); };
