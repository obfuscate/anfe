#pragma once

struct IVector2 : public XMINT2
{
	IVector2() noexcept : XMINT2(0, 0) {}
	constexpr explicit IVector2(int32_t x) noexcept : XMINT2(x, x) {}
	constexpr IVector2(int32_t x, int32_t y) noexcept : XMINT2(x, y) {}

	IVector2(const IVector2&) = default;
	IVector2& operator=(const IVector2&) = default;

	IVector2(IVector2&&) = default;
	IVector2& operator=(IVector2&&) = default;

	// Comparison operators
	bool operator == (const IVector2& v) const noexcept { return ((x == v.x) && (y == v.y)); }
	bool operator != (const IVector2& v) const noexcept { return ((x != v.x) || (y != v.y)); }

	// Assignment operators
	IVector2& operator+= (const IVector2& v) noexcept { x += v.x; y += v.y; return *this; }
	IVector2& operator-= (const IVector2& v) noexcept { x -= v.x; y -= v.y; return *this; }
	IVector2& operator*= (const IVector2& v) noexcept { x *= v.x; y *= v.y; return *this; }
	IVector2& operator+= (int32_t s) noexcept { x += s; y += s; return *this; }
	IVector2& operator*= (int32_t s) noexcept { x *= s; y *= s; return *this; }
	//IVector2& operator/= (int32_t s) noexcept { x /= s; y /= s; return *this; }

	// Unary operators
	IVector2 operator+ () const noexcept { return *this; }
	IVector2 operator- () const noexcept { return IVector2(-x, -y); }

	void Clamp(const IVector2& vmin, const IVector2& vmax) noexcept;
	void Clamp(const IVector2& vmin, const IVector2& vmax, IVector2& result) const noexcept;

	// Static functions
	static void Min(const IVector2& v1, const IVector2& v2, IVector2& result) noexcept;
	static IVector2 Min(const IVector2& v1, const IVector2& v2) noexcept;

	static void Max(const IVector2& v1, const IVector2& v2, IVector2& result) noexcept;
	static IVector2 Max(const IVector2& v1, const IVector2& v2) noexcept;

	static constexpr IVector2 zero() noexcept { return IVector2(0, 0); }
	static constexpr IVector2 one() noexcept { return IVector2(1, 1); }
	static constexpr IVector2 unitX() noexcept { return IVector2(1, 0); }
	static constexpr IVector2 unitY() noexcept { return IVector2(0, 1); }
};

FORCE_INLINE void IVector2::Clamp(const IVector2& vmin, const IVector2& vmax) noexcept
{
	using namespace DirectX;
	const XMVECTOR v1 = XMLoadSInt2(this);
	const XMVECTOR v2 = XMLoadSInt2(&vmin);
	const XMVECTOR v3 = XMLoadSInt2(&vmax);
	const XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreSInt2(this, X);
}

FORCE_INLINE void IVector2::Clamp(const IVector2& vmin, const IVector2& vmax, IVector2& result) const noexcept
{
	using namespace DirectX;
	const XMVECTOR v1 = XMLoadSInt2(this);
	const XMVECTOR v2 = XMLoadSInt2(&vmin);
	const XMVECTOR v3 = XMLoadSInt2(&vmax);
	const XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreSInt2(&result, X);
}

// Binary operators
FORCE_INLINE IVector2 operator+ (const IVector2& V1, const IVector2& V2) noexcept { return IVector2(V1.x + V2.x, V1.y + V2.y); }
FORCE_INLINE IVector2 operator- (const IVector2& V1, const IVector2& V2) noexcept { return IVector2(V1.x - V2.x, V1.y - V2.y); }
FORCE_INLINE IVector2 operator* (const IVector2& V1, const IVector2& V2) noexcept { return IVector2(V1.x * V2.x, V1.y * V2.y); }
FORCE_INLINE IVector2 operator* (const IVector2& v, int32_t s) noexcept { return IVector2(v.x * s, v.y * s); }
//FORCE_INLINE IVector2 operator/ (const IVector2& V1, const IVector2& V2) noexcept { return IVector2(V1.x / V2.x, V1.y / V2.y); }
//FORCE_INLINE IVector2 operator/ (const IVector2& v, float s) noexcept { return IVector2(v.x / s, v.y / s); }
FORCE_INLINE IVector2 operator* (int32_t s, const IVector2& v) noexcept { return IVector2(s * v.x, s * v.y); };
