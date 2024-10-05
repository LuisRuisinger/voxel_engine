// Daniel Shervheim, 2019
// danielshervheim.com
// modifications by Luis S. Ruisinger, 2024

#version 330 core

// Normalization factors from the precomputation phase.
const vec2 RAYLEIGH_NORM = vec2(0.0, 0.05588319);
const vec2 MIE_NORM = vec2(0.0, .02527083);

// Spectral irradiance and spectral to RGB conversion constants from
// the precomputation phase.
const vec3 SPECTRAL_IRRADIANCE = vec3(1.526, 1.91, 2.08) / 10.0;
const vec3 SPECTRAL_TO_RGB = vec3(133.3209, 88.51855, 112.7552);

const float SUN_ANGULAR_RADIUS = 0.004675034;

uniform vec3 lightDir;

uniform sampler2D rayleighTexture;
uniform sampler2D mieTexture;

uniform float exposure;
uniform bool rayleighEnabled;
uniform bool mieEnabled;
uniform float mieG;

uniform vec3 camera;

in vec3 WorldPosition;

out vec4 FragColor;

float RayleighPhaseFunction(float cosTheta) {
    // Original rayleigh phase function.
    // return 0.75 * (1 + pow(cosTheta, 2));

    // Modified to better account for sun-view azimuth as described in Section 4.1 of:
    // http://publications.lib.chalmers.se/records/fulltext/203057/203057.pdf
    return 0.8 * (1.4 + 0.5*cosTheta);
}

float MiePhaseFunction(float cosTheta, float g) {
    float g2 = g * g;
    float t2 = cosTheta * cosTheta;
    float result = 3.0 / 2.0;
    result *= (1.0 - g2) / (2.0 + g2);
    result *= (1.0 + t2) / pow(1.0 + g2 - 2.0*g*t2, 3.0/2.0);
    return result;
}

void main() {
    vec3 viewDir = normalize(WorldPosition - camera);

    // Calculate the view-zenith and sun-zenith angles.
    // negative for above 0 values in y i suppose
    // therefore because of they are normalized the values range from [-1, 0]
    // with 0 if y is 0, else decline in value towards -1

    // but all values are equally used in the upper sphere => we can just interate
    // over the dot product
    float cosV = dot(viewDir, vec3(0, -1, 0));
    float cosL = dot(lightDir, vec3(0, 1, 0));

    // Convert the angles to texture coordinates using the parameterization function.
    // Note: we use abs+sign to avoid negative roots!

    // u declined the further y goes up
    // at y = 0 => u = 0.5
    // at the highest y angle <=> inverse of (0, -1, 0) : (0, 1, 0) the limit of the dot
    // product is reached resulting in -1 => yields 0 for u
    // therefore we only observe the lower 50% of the texture to calculate the ambient texture
    float u = 0.5 * (1.0 + sign(cosV)*pow(abs(cosV), 1.0/3.0));
    float v = 0.5 * (1.0 + sign(cosL)*pow(abs(cosL), 1.0/3.0));

    // Sample the textures.
    vec3 rayleigh = texture(rayleighTexture, vec2(u, v)).rgb;
    vec3 mie = texture(mieTexture, vec2(u, v)).rgb;

    // Remap the values.
    rayleigh = rayleigh*(RAYLEIGH_NORM.y-RAYLEIGH_NORM.x) + RAYLEIGH_NORM.x;
    mie = mie*(MIE_NORM.y-MIE_NORM.x) + MIE_NORM.x;

    // Calculate the view-sun angle for the phase function.
    // Note: we clamp it between [0, 1] or else we would get the sun
    // on both sides of the light direction.
    float cosTheta = dot(viewDir, lightDir);

    // changed from saturate
    cosTheta = clamp(cosTheta, 0.0F, 1.0F);

    // Apply the phase function.
    rayleigh *= RayleighPhaseFunction(cosTheta);
    mie *= MiePhaseFunction(cosTheta, mieG);

    // Compute the scattering, and apply the spectral irradiance to
    // get the spectral radiance for this fragment.
    vec3 radiance = vec3(0.0);
    radiance += rayleighEnabled ? rayleigh : vec3(0.0);
    radiance += mieEnabled ? mie : vec3(0.0);
    radiance *= SPECTRAL_IRRADIANCE * vec3(exposure);

    // Multiply by the SPECTRAL_TO_RGB conversion constants to convert
    // the spectral radiance to RGB values.
    vec3 rgb = radiance * SPECTRAL_TO_RGB;

    if (acos(cosTheta) < SUN_ANGULAR_RADIUS)
    {
        // TODO: this is not physically correct. It only works for exposure < 1.
        // Technically it should be multiplied by the transmittance.
        rgb /= SPECTRAL_IRRADIANCE * vec3(exposure);
    }

    // Tonemap the resulting RGB samples into a valid RGB range.
    rgb = pow(vec3(1.0) - exp(-rgb), vec3(1.0/2.2));

    FragColor = vec4(rgb, 1.0);
}
