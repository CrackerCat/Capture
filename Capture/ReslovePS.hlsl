Texture2D src;//register(t0)
SamplerState point_sampler;//register(s0)

float4 main(float2 iTex:TEXCOORD) : SV_TARGET
{
	return src.Sample(point_sampler,iTex).bgra;
}