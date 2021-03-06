#include "./Common.hlsli"

//	https://docs.microsoft.com/en-us/windows/desktop/direct3d12/dynamic-indexing-using-hlsl-5-1

StructuredBuffer<float3> cPosition: register(t0);
StructuredBuffer<float4> cColor: register(t1);

AppendStructuredBuffer<vertex_t> cVerts: register(u0);


[numthreads(1, 1, 1)]
void main( uint3 threadId : SV_DispatchThreadID, uint3 groupID: SV_GroupThreadID )
{
	uint count, _;
	cPosition.GetDimensions(count, _);

	for(uint i = 0; i < count; i++) {
		vertex_t v;
		
		v.position = float4(cPosition[i], i);
		v.color = cColor[i];
		
		
		cVerts.Append(v);
	}
}