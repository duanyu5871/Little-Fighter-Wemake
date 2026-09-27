#include <skinning_pars_vertex>

uniform float uOutline;
uniform vec3 uCenter;

void main() {
  vec3 dir = position - uCenter;
  float len = length(dir);
  vec3 transformed = position + (len > 1e-6 ? dir * (uOutline / len) : vec3(0.0));

  #include <skinbase_vertex>
  #include <skinning_vertex>

  gl_Position = projectionMatrix * modelViewMatrix * vec4(transformed, 1.0);
}
