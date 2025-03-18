#pragma once

struct UVector2 : public XMUINT2
{
	UVector2() noexcept : XMUINT2(0, 0) {}
	constexpr explicit UVector2(uint32_t ix) noexcept : XMUINT2(ix, ix) {}
	constexpr UVector2(uint32_t ix, uint32_t iy) noexcept : XMUINT2(ix, iy) {}

	UVector2(const UVector2&) = default;
	UVector2& operator=(const UVector2&) = default;

	UVector2(UVector2&&) = default;
	UVector2& operator=(UVector2&&) = default;

	// Comparison operators
	bool operator == (const UVector2& v) const noexcept { return ((x == v.x) && (y == v.y)); }
	bool operator != (const UVector2& v) const noexcept { return ((x != v.x) || (y != v.y)); }

	// Assignment operators
	UVector2& operator+= (const UVector2& v) noexcept { x += v.x; y += v.y; return *this; }
	UVector2& operator-= (const UVector2& v) noexcept { x -= v.x; y -= v.y; return *this; }
	UVector2& operator*= (const UVector2& v) noexcept { x *= v.x; y *= v.y; return *this; }
	UVector2& operator+= (uint32_t s) noexcept { x += s; y += s; return *this; }
	UVector2& operator*= (uint32_t s) noexcept { x *= s; y *= s; return *this; }
	//UVector2& operator/= (uint32_t s) noexcept { x /= s; y /= s; return *this; }

	// Unary operators
	UVector2 operator+ () const noexcept { return *this; }
	//UVector2 operator- () const noexcept { return UVector2(-x, -y); }

	void Clamp(const UVector2& vmin, const UVector2& vmax) noexcept;
	void Clamp(const UVector2& vmin, const UVector2& vmax, UVector2& result) const noexcept;

	// Static functions
	static void Min(const UVector2& v1, const UVector2& v2, UVector2& result) noexcept;
	static UVector2 Min(const UVector2& v1, const UVector2& v2) noexcept;

	static void Max(const UVector2& v1, const UVector2& v2, UVector2& result) noexcept;
	static UVector2 Max(const UVector2& v1, const UVector2& v2) noexcept;

	static constexpr UVector2 zero() noexcept { return UVector2(0, 0); }
	static constexpr UVector2 one() noexcept { return UVector2(1, 1); }
	static constexpr UVector2 unitX() noexcept { return UVector2(1, 0); }
	static constexpr UVector2 unitY() noexcept { return UVector2(0, 1); }
};

FORCE_INLINE void UVector2::Clamp(const UVector2& vmin, const UVector2& vmax) noexcept
{
	using namespace DirectX;
	const XMVECTOR v1 = XMLoadUInt2(this);
	const XMVECTOR v2 = XMLoadUInt2(&vmin);
	const XMVECTOR v3 = XMLoadUInt2(&vmax);
	const XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreUInt2(this, X);
}

FORCE_INLINE void UVector2::Clamp(const UVector2& vmin, const UVector2& vmax, UVector2& result) const noexcept
{
	using namespace DirectX;
	const XMVECTOR v1 = XMLoadUInt2(this);
	const XMVECTOR v2 = XMLoadUInt2(&vmin);
	const XMVECTOR v3 = XMLoadUInt2(&vmax);
	const XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreUInt2(&result, X);
}

// Binary operators
FORCE_INLINE UVector2 operator+ (const UVector2& V1, const UVector2& V2) noexcept { return UVector2(V1.x + V2.x, V1.y + V2.y); }
FORCE_INLINE UVector2 operator- (const UVector2& V1, const UVector2& V2) noexcept { return UVector2(V1.x - V2.x, V1.y - V2.y); }
FORCE_INLINE UVector2 operator* (const UVector2& V1, const UVector2& V2) noexcept { return UVector2(V1.x * V2.x, V1.y * V2.y); }
FORCE_INLINE UVector2 operator* (const UVector2& v, uint32_t s) noexcept { return UVector2(v.x * s, v.y * s); }
//FORCE_INLINE UVector2 operator/ (const UVector2& V1, const UVector2& V2) noexcept { return UVector2(V1.x / V2.x, V1.y / V2.y); }
//FORCE_INLINE UVector2 operator/ (const UVector2& v, float s) noexcept { return UVector2(v.x / s, v.y / s); }
FORCE_INLINE UVector2 operator* (uint32_t s, const UVector2& v) noexcept { return UVector2(s * v.x, s * v.y); };

