Texture2D src;//register(t0)
SamplerState src_sampler;//register(s0)


float4 main(float2 iTex:TEXCOORD) : SV_TARGET
{
	float4 color = src.Sample(src_sampler,iTex);
	return color.bgra;
}