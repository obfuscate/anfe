#pragma once

struct UVector4 : public XMUINT4
{
	UVector4() noexcept : XMUINT4(0, 0, 0, 0) {}
	constexpr explicit UVector4(const int32_t x) noexcept : XMUINT4(x, x, x, x) {}
	constexpr UVector4(const int32_t x, const int32_t y, const int32_t z, const int32_t w) noexcept : XMUINT4(x, y, z, w) {}
	UVector4(const UVector3& v, const int32_t w) noexcept : XMUINT4(v.x, v.y, v.z, w) {}
	UVector4(const UVector2& v, const int32_t z, const int32_t w) noexcept : XMUINT4(v.x, v.y, z, w) {}

	UVector4(const UVector4&) = default;
	UVector4& operator=(const UVector4&) = default;

	UVector4(UVector4&&) = default;
	UVector4& operator=(UVector4&&) = default;

	// Comparison operators
	bool operator == (const UVector4& v) const noexcept { return ((x == v.x) && (y == v.y) && (z == v.z) && (w == v.w)); }
	bool operator != (const UVector4& v) const noexcept { return ((x != v.x) || (y != v.y) || (z != v.z) || (w != v.w)); }

	// Assignment operators
	UVector4& operator+= (const UVector4& v) noexcept { x += v.x; y += v.y; z += v.z; z += v.z; w += v.w; return *this; }
	UVector4& operator-= (const UVector4& v) noexcept { x -= v.x; y -= v.y; z -= v.z; z -= v.z; w -= v.w; return *this; }
	UVector4& operator*= (const UVector4& v) noexcept { x *= v.x; y *= v.y; z *= v.z; z *= v.z; w *= v.w; return *this; }
	UVector4& operator+= (uint32_t s) noexcept { x += s; y += s; z += s; w += s; return *this; }
	UVector4& operator*= (uint32_t s) noexcept { x *= s; y *= s; z *= s; w *= s; return *this; }
	//UVector4& operator/= (int32_t s) noexcept { x /= s; y /= s; z /= s; w /= s; return *this; }

	// Unary operators
	UVector4 operator+ () const noexcept { return *this; }
	//UVector4 operator- () const noexcept { return UVector4(-x, -y, -z, -w); }

	void Clamp(const UVector4& vmin, const UVector4& vmax) noexcept;
	void Clamp(const UVector4& vmin, const UVector4& vmax, UVector4& result) const noexcept;

	// Static functions
	static void Min(const UVector4& v1, const UVector4& v2, UVector4& result) noexcept;
	static UVector4 Min(const UVector4& v1, const UVector4& v2) noexcept;

	static void Max(const UVector4& v1, const UVector4& v2, UVector4& result) noexcept;
	static UVector4 Max(const UVector4& v1, const UVector4& v2) noexcept;

	static constexpr UVector4 zero() noexcept { return UVector4(0, 0, 0, 0); }
	static constexpr UVector4 one() noexcept { return UVector4(1, 1, 1, 1); }
	static constexpr UVector4 unitX() noexcept { return UVector4(1, 0, 0, 0); }
	static constexpr UVector4 unitY() noexcept { return UVector4(0, 1, 0, 0); }
	static constexpr UVector4 unitZ() noexcept { return UVector4(0, 0, 1, 0); }
};

FORCE_INLINE void UVector4::Clamp(const UVector4& vmin, const UVector4& vmax) noexcept
{
	using namespace DirectX;
	const XMVECTOR v1 = XMLoadUInt4(this);
	const XMVECTOR v2 = XMLoadUInt4(&vmin);
	const XMVECTOR v3 = XMLoadUInt4(&vmax);
	const XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreUInt4(this, X);
}

FORCE_INLINE void UVector4::Clamp(const UVector4& vmin, const UVector4& vmax, UVector4& result) const noexcept
{
	using namespace DirectX;
	const XMVECTOR v1 = XMLoadUInt4(this);
	const XMVECTOR v2 = XMLoadUInt4(&vmin);
	const XMVECTOR v3 = XMLoadUInt4(&vmax);
	const XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreUInt4(&result, X);
}

// Binary operators
FORCE_INLINE UVector4 operator+ (const UVector4& V1, const UVector4& V2) noexcept { return UVector4(V1.x + V2.x, V1.y + V2.y, V1.z + V2.z, V1.w + V2.w); }
FORCE_INLINE UVector4 operator- (const UVector4& V1, const UVector4& V2) noexcept { return UVector4(V1.x - V2.x, V1.y - V2.y, V1.z - V2.z, V1.w - V2.w); }
FORCE_INLINE UVector4 operator* (const UVector4& V1, const UVector4& V2) noexcept { return UVector4(V1.x * V2.x, V1.y * V2.y, V1.z * V2.z, V1.w * V2.w); }
FORCE_INLINE UVector4 operator* (const UVector4& v, uint32_t s) noexcept { return UVector4(v.x * s, v.y * s, v.z * s, v.w * s); }
//FORCE_INLINE UVector4 operator/ (const UVector4& V1, const UVector4& V2) noexcept { return UVector4(V1.x / V2.x, V1.y / V2.y, V1.z / V2.z); }
//FORCE_INLINE UVector4 operator/ (const UVector4& v, float s) noexcept { return UVector4(v.x / s, v.y / s, v.z / s); }
FORCE_INLINE UVector4 operator* (uint32_t s, const UVector4& v) noexcept { return UVector4(s * v.x, s * v.y, s * v.z, s * v.w); };
