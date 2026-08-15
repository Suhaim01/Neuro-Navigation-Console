#version 330 core

uniform sampler3D uVolume;

// per pixel value
in vec2 vUV;

out vec4 fragColor;

void main()
{
  // Mid-volume axial plane for steps 1–3. Orientations land in step 4.
  float intensity = texture(uVolume, vec3(vUV, 0.5)).r;
  // Crude display scale until WW/WL (step 5).
  float g = clamp(intensity / 255.0, 0.0, 1.0);
  fragColor = vec4(g, g, g, 1.0);
}
