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

// per pixel value
in vec2 vUV;

out vec4 fragColor;

void main()
{
  // Build a point on the patient-space mid-plane, then
  // map to voxel indices with inverse(voxelToImage).
  vec3 patient;

  // building world position for given vUV and slice
  if (uOrientation == 0) {
    // Axial: fixed z
    patient = vec3(
        mix(uPatientMin.x, uPatientMax.x, vUV.x),
        mix(uPatientMin.y, uPatientMax.y, vUV.y),
        mix(uPatientMin.z, uPatientMax.z, uSlice));
  } else if (uOrientation == 1) {
    // Coronal: fixed y
    patient = vec3(
        mix(uPatientMin.x, uPatientMax.x, vUV.x),
        mix(uPatientMin.y, uPatientMax.y, uSlice),
        mix(uPatientMin.z, uPatientMax.z, vUV.y));
  } else {
    // Sagittal: fixed x
    patient = vec3(
        mix(uPatientMin.x, uPatientMax.x, uSlice),
        mix(uPatientMin.y, uPatientMax.y, vUV.x),
        mix(uPatientMin.z, uPatientMax.z, vUV.y));
  }

  vec4 voxelH = uVoxelFromImage * vec4(patient, 1.0);
  vec3 texCoord = (voxelH.xyz + 0.5) / uVolSize;
  if (any(lessThan(texCoord, vec3(0.0))) || any(greaterThan(texCoord, vec3(1.0)))) {
    fragColor = vec4(0.0, 0.0, 0.0, 1.0);
    return;
  }

  float intensity = texture(uVolume, texCoord).r;
  // WW/WL: map [WL - WW/2, WL + WW/2] → [0, 1]
  float g = clamp((intensity - (uWindowLevel - 0.5 * uWindowWidth)) / uWindowWidth, 0.0, 1.0);
  fragColor = vec4(g, g, g, 1.0);
}
