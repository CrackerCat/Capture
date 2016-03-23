void main(float4 pos : POSITION,float2 tex:TEXCOORD ,out float2 oTex : TEXCOORD, out float4 oPos : SV_POSITION)
{
	oTex = tex;
	oPos = pos;
}