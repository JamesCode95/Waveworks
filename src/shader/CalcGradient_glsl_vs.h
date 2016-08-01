R"glsl(
//------------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------------

uniform vec4 g_Scales;
uniform vec4 g_OneTexel_Left;
uniform vec4 g_OneTexel_Right;
uniform vec4 g_OneTexel_Back;
uniform vec4 g_OneTexel_Front;

uniform sampler2D g_textureDisplacementMap;

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
