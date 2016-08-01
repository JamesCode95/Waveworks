R"glsl(
//------------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------------

uniform vec4 g_Scales;
uniform vec4 g_OneTexel_Left;
uniform vec4 g_OneTexel_Right;
uniform vec4 g_OneTexel_Back;
uniform vec4 g_OneTexel_Front;

uniform sampler2D g_samplerDisplacementMap;

varying float2 vInterpTexCoord;

void main()
{
	// Sample neighbour texels
	float3 displace_left	= texture(g_samplerDisplacementMap, vInterpTexCoord.xy + g_OneTexel_Left.xy).rgb;
	float3 displace_right	= texture(g_samplerDisplacementMap, vInterpTexCoord.xy + g_OneTexel_Right.xy).rgb;
	float3 displace_back	= texture(g_samplerDisplacementMap, vInterpTexCoord.xy + g_OneTexel_Back.xy).rgb;
	float3 displace_front	= texture(g_samplerDisplacementMap, vInterpTexCoord.xy + g_OneTexel_Front.xy).rgb;
	
	// -------- Do not store the actual normal value, instead, it preserves two differential values.
	float2 gradient = float2(-(displace_right.z - displace_left.z) / max(0.01,1.0 + g_Scales.y*(displace_right.x - displace_left.x)), -(displace_front.z - displace_back.z) / max(0.01,1.0+g_Scales.y*(displace_front.y - displace_back.y)));
	//float2 gradient = {-(displace_right.z - displace_left.z), -(displace_front.z - displace_back.z) };
	
	// Calculate Jacobian corelation from the partial differential of displacement field
	float2 Dx = (displace_right.xy - displace_left.xy) * g_Scales.x;
	float2 Dy = (displace_front.xy - displace_back.xy) * g_Scales.x;
	float J = (1.0f + Dx.x) * (1.0f + Dy.y) - Dx.y * Dy.x;

	// Output
	gl_FragColor = float4(gradient, J, 0);
}
)glsl";
