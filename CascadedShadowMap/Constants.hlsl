static const float PI = 3.14159265f;

static const float4 LIGHTDIR = normalize(float4(1.0f, -0.5f, 1.0f, 0.0f));

static const uint MAXINSTANCES = 50;

static const uint NFRUSTA = 1;

static const matrix MATRIX = {
	0.1f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.1f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.05f, 0.0f,
	0.0f, 0.0f, 0.5f, 1.0f
};

static const float INVFRUSTA = 1.0f / NFRUSTA;

static const float DSHADOW = 1.0f / 2048.0f;

static const float CLIPMAP_DX = 1.0f / 255.0f;