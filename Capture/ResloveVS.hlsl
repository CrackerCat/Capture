float2 TexCoordFromPos(float4 pos)
{
	float2 tex = pos.xy / 2;
	tex.y *= -1;
	tex += 0.5;
	return tex;
}

void main( float4 pos : POSITION,out float2 oTex:TEXCOORD,out float4 oPos:SV_POSITION)
{
	oTex = TexCoordFromPos(pos);
	oPos = pos;
}