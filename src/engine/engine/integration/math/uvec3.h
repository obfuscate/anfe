#pragma once

struct UVector3 : public XMUINT3
{
	UVector3() noexcept : XMUINT3(0, 0, 0) {}
	constexpr explicit UVector3(const uint32_t x) noexcept : XMUINT3(x, x, x) {}
	constexpr UVector3(const uint32_t x, const uint32_t y, const uint32_t z) noexcept : XMUINT3(x, y, z) {}
	UVector3(const UVector2& v, const uint32_t z) noexcept : XMUINT3(v.x, v.y, z) {}

	UVector3(const UVector3&) = default;
	UVector3& operator=(const UVector3&) = default;

	UVector3(UVector3&&) = default;
	UVector3& operator=(UVector3&&) = default;

	// Comparison operators
	bool operator == (const UVector3& v) const noexcept { return ((x == v.x) && (y == v.y) && (z == v.z)); }
	bool operator != (const UVector3& v) const noexcept { return ((x != v.x) || (y != v.y) || (z != v.z)); }

	// Assignment operators
	UVector3& operator+= (const UVector3& v) noexcept { x += v.x; y += v.y; z += v.z; return *this; }
	UVector3& operator-= (const UVector3& v) noexcept { x -= v.x; y -= v.y; z -= v.z; return *this; }
	UVector3& operator*= (const UVector3& v) noexcept { x *= v.x; y *= v.y; z *= v.z; return *this; }
	UVector3& operator+= (uint32_t s) noexcept { x += s; y += s; z += s; return *this; }
	UVector3& operator*= (uint32_t s) noexcept { x *= s; y *= s; z *= s; return *this; }
	//UVector3& operator/= (uint32_t s) noexcept { x /= s; y /= s; z /= s; return *this; }

	// Unary operators
	UVector3 operator+ () const noexcept { return *this; }
	//UVector3 operator- () const noexcept { return UVector3(-x, -y, -z); }

	void Clamp(const UVector3& vmin, const UVector3& vmax) noexcept;
	void Clamp(const UVector3& vmin, const UVector3& vmax, UVector3& result) const noexcept;

	// Static functions
	static void Min(const UVector3& v1, const UVector3& v2, UVector3& result) noexcept;
	static UVector3 Min(const UVector3& v1, const UVector3& v2) noexcept;

	static void Max(const UVector3& v1, const UVector3& v2, UVector3& result) noexcept;
	static UVector3 Max(const UVector3& v1, const UVector3& v2) noexcept;

	static constexpr UVector3 zero() noexcept { return UVector3(0, 0, 0); }
	static constexpr UVector3 one() noexcept { return UVector3(1, 1, 1); }
	static constexpr UVector3 unitX() noexcept { return UVector3(1, 0, 0); }
	static constexpr UVector3 unitY() noexcept { return UVector3(0, 1, 0); }
	static constexpr UVector3 unitZ() noexcept { return UVector3(0, 0, 1); }
};

FORCE_INLINE void UVector3::Clamp(const UVector3& vmin, const UVector3& vmax) noexcept
{
	using namespace DirectX;
	const XMVECTOR v1 = XMLoadUInt3(this);
	const XMVECTOR v2 = XMLoadUInt3(&vmin);
	const XMVECTOR v3 = XMLoadUInt3(&vmax);
	const XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreUInt3(this, X);
}

FORCE_INLINE void UVector3::Clamp(const UVector3& vmin, const UVector3& vmax, UVector3& result) const noexcept
{
	using namespace DirectX;
	const XMVECTOR v1 = XMLoadUInt3(this);
	const XMVECTOR v2 = XMLoadUInt3(&vmin);
	const XMVECTOR v3 = XMLoadUInt3(&vmax);
	const XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreUInt3(&result, X);
}

// Binary operators
FORCE_INLINE UVector3 operator+ (const UVector3& V1, const UVector3& V2) noexcept { return UVector3(V1.x + V2.x, V1.y + V2.y, V1.z + V2.z); }
FORCE_INLINE UVector3 operator- (const UVector3& V1, const UVector3& V2) noexcept { return UVector3(V1.x - V2.x, V1.y - V2.y, V1.z - V2.z); }
FORCE_INLINE UVector3 operator* (const UVector3& V1, const UVector3& V2) noexcept { return UVector3(V1.x * V2.x, V1.y * V2.y, V1.z * V2.z); }
FORCE_INLINE UVector3 operator* (const UVector3& v, uint32_t s) noexcept { return UVector3(v.x * s, v.y * s, v.z * s); }
//FORCE_INLINE UVector3 operator/ (const UVector3& V1, const UVector3& V2) noexcept { return UVector3(V1.x / V2.x, V1.y / V2.y, V1.z / V2.z); }
//FORCE_INLINE UVector3 operator/ (const UVector3& v, float s) noexcept { return UVector3(v.x / s, v.y / s, v.z / s); }
FORCE_INLINE UVector3 operator* (uint32_t s, const UVector3& v) noexcept { return UVector3(s * v.x, s * v.y, s * v.z); };
