R"glsl(
//------------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------------

uniform vec4 g_DissipationFactors;
uniform vec4 g_SourceComponents;
uniform vec4 g_UVOffsets;

uniform sampler2D g_samplerDisplacementMap;

varying float2 vInterpTexCoord;

attribute float4 vInPos;
attribute float2 vInTexCoord;

void main()
{
    // No need to do matrix transform.
    gl_Position = vInPos;
    
	// Pass through general texture coordinate.
    vInterpTexCoord = vInTexCoord;
}
)glsl";