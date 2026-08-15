// shader’s job is only to compute the right texCoord for each pixel

#version 330 core

uniform sampler3D uVolume;
uniform mat4 uVoxelFromImage;
uniform vec3 uVolSize;
uniform vec3 uPatientMin;
uniform vec3 uPatientMax;
// 0 = axial (fixed patient z), 1 = coronal (fixed y), 2 = sagittal (fixed x)
uniform int uOrientation;
uniform float uSlice;
uniform float uWindowLevel;
uniform float uWindowWidth;
uniform vec2 uCrossUV;
uniform float uZoom;

// per pixel value
in vec2 vUV;

out vec4 fragColor;

void main()
{
  // Zoom about view center: map screen UV → patient-normalized UV.
  vec2 uvSample = 0.5 + (vUV - vec2(0.5)) / uZoom;

  vec3 patient;

  // building world position for given vUV and slice
  if (uOrientation == 0) {
    // Axial: fixed z
    patient = vec3(
        mix(uPatientMin.x, uPatientMax.x, uvSample.x),
        mix(uPatientMin.y, uPatientMax.y, uvSample.y),
        mix(uPatientMin.z, uPatientMax.z, uSlice));
  } else if (uOrientation == 1) {
    // Coronal: fixed y
    patient = vec3(
        mix(uPatientMin.x, uPatientMax.x, uvSample.x),
        mix(uPatientMin.y, uPatientMax.y, uSlice),
        mix(uPatientMin.z, uPatientMax.z, uvSample.y));
  } else {
    // Sagittal: fixed x
    patient = vec3(
        mix(uPatientMin.x, uPatientMax.x, uSlice),
        mix(uPatientMin.y, uPatientMax.y, uvSample.x),
        mix(uPatientMin.z, uPatientMax.z, uvSample.y));
  }

  vec4 voxelH = uVoxelFromImage * vec4(patient, 1.0);
  vec3 texCoord = (voxelH.xyz + 0.5) / uVolSize;
  if (any(lessThan(texCoord, vec3(0.0))) || any(greaterThan(texCoord, vec3(1.0)))) {
    fragColor = vec4(0.0, 0.0, 0.0, 1.0);
  } else {
    float intensity = texture(uVolume, texCoord).r;
    float g = clamp((intensity - (uWindowLevel - 0.5 * uWindowWidth)) / uWindowWidth, 0.0, 1.0);
    fragColor = vec4(g, g, g, 1.0);
  }

  // Crosshair in screen UV (focus patient UV projected through zoom).
  vec2 crossScreen = 0.5 + (uCrossUV - vec2(0.5)) * uZoom;
  const float halfWidth = 0.003;
  if (abs(vUV.x - crossScreen.x) < halfWidth || abs(vUV.y - crossScreen.y) < halfWidth) {
    fragColor = vec4(1.0, 0.85, 0.15, 1.0);
  }
}
