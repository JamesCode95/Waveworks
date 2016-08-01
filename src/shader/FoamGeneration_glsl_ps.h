R"glsl(
//------------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------------

uniform vec4 g_DissipationFactors;
uniform vec4 g_SourceComponents;
uniform vec4 g_UVOffsets;

uniform sampler2D g_samplerDisplacementMap;

varying float2 vInterpTexCoord;

// at 1st rendering step, the folding and the accumulated foam values are being read from gradient map (components z and w),
// blurred by X, summed, faded and written to foam energy map 

// at 2nd rendering step, the accumulated foam values are being read from foam energy texture,
// blurred by Y and written to w component of gradient map

void main()
{

	float2 UVoffset = g_UVOffsets.xy*g_DissipationFactors.x;

	// blur with variable size kernel is done by doing 4 bilinear samples, 
	// each sample is slightly offset from the center point
	float foamenergy1	= dot(g_SourceComponents, texture(g_samplerEnergyMap, vInterpTexCoord.xy + UVoffset));
	float foamenergy2	= dot(g_SourceComponents, texture(g_samplerEnergyMap, vInterpTexCoord.xy - UVoffset));
	float foamenergy3	= dot(g_SourceComponents, texture(g_samplerEnergyMap, vInterpTexCoord.xy + UVoffset*2.0));
	float foamenergy4	= dot(g_SourceComponents, texture(g_samplerEnergyMap, vInterpTexCoord.xy - UVoffset*2.0));
	
	float folding = max(0,texture(g_samplerEnergyMap, vInterpTexCoord.xy).z);

	float energy = g_DissipationFactors.y*((foamenergy1 + foamenergy2 + foamenergy3 + foamenergy4)*0.25 + max(0,(1.0-folding-g_DissipationFactors.w))*g_DissipationFactors.z);

	energy = min(1.0,energy);

	// Output
	gl_FragColor = float4(energy,energy,energy,energy);
}
)glsl";
