#ifndef VECTOR_MATH_H
#define VECTOR_MATH_H

#include <math.h>

//for debug/printing functions
#include <stdio.h>

typedef struct
{
  double x;
  double y;
  double z;
} vec3;

typedef struct
{
  double x;
  double y;
  double z;
  double w;
} vec4;

typedef struct
{
  vec3 v0;
  vec3 v1;
  vec3 v2;
} mat3;

typedef struct
{
  vec4 v0;
  vec4 v1;
  vec4 v2;
  vec4 v3;
} mat4;

double vec3_len(const vec3 * vec)
{
  return sqrt(vec->x * vec->x + vec->y * vec->y + vec->z * vec->z);
}

void vec3_normalize(vec3 * vec)
{
  double len = vec3_len(vec);
  vec->x /= len;
  vec->y /= len;
  vec->z /= len;
}

void vec3_cross(vec3 * out, const vec3 * first, const vec3 * second)
{
  out->x = first->y * second->z - first->z * second->y;
  out->y = first->z * second->x - first->x * second->z;
  out->z = first->x * second->y - first->y * second->x;
}

void mat4_make_translation(mat4 * mat, const vec3 * translate)
{
  mat->v0 = (vec4){ 1.0, 0.0, 0.0, 0.0 };
  mat->v1 = (vec4){ 0.0, 1.0, 0.0, 0.0 };
  mat->v2 = (vec4){ 0.0, 0.0, 1.0, 0.0 };
  mat->v3 = (vec4){ translate->x, translate->y, translate->z, 1.0 };
}

void mat4_mult(mat4 * a, const mat4 * b)
{
  double a11 = a->v0.x, a12 = a->v1.x, a13 = a->v2.x, a14 = a->v3.x;
  double a21 = a->v0.y, a22 = a->v1.y, a23 = a->v2.y, a24 = a->v3.y;
  double a31 = a->v0.z, a32 = a->v1.z, a33 = a->v2.z, a34 = a->v3.z;
  double a41 = a->v0.w, a42 = a->v1.w, a43 = a->v2.w, a44 = a->v3.w;

  double b11 = b->v0.x, b12 = b->v1.x, b13 = b->v2.x, b14 = b->v3.x;
  double b21 = b->v0.y, b22 = b->v1.y, b23 = b->v2.y, b24 = b->v3.y;
  double b31 = b->v0.z, b32 = b->v1.z, b33 = b->v2.z, b34 = b->v3.z;
  double b41 = b->v0.w, b42 = b->v1.w, b43 = b->v2.w, b44 = b->v3.w;

  a->v0.x = a11 * b11 + a12 * b21 + a13 * b31 + a14 * b41;
  a->v1.x = a11 * b12 + a12 * b22 + a13 * b32 + a14 * b42;
  a->v2.x = a11 * b13 + a12 * b23 + a13 * b33 + a14 * b43;
  a->v3.x = a11 * b14 + a12 * b24 + a13 * b34 + a14 * b44;

  a->v0.y = a21 * b11 + a22 * b21 + a23 * b31 + a24 * b41;
  a->v1.y = a21 * b12 + a22 * b22 + a23 * b32 + a24 * b42;
  a->v2.y = a21 * b13 + a22 * b23 + a23 * b33 + a24 * b43;
  a->v3.y = a21 * b14 + a22 * b24 + a23 * b34 + a24 * b44;

  a->v0.z = a31 * b11 + a32 * b21 + a33 * b31 + a34 * b41;
  a->v1.z = a31 * b12 + a32 * b22 + a33 * b32 + a34 * b42;
  a->v2.z = a31 * b13 + a32 * b23 + a33 * b33 + a34 * b43;
  a->v3.z = a31 * b14 + a32 * b24 + a33 * b34 + a34 * b44;

  a->v0.w = a41 * b11 + a42 * b21 + a43 * b31 + a44 * b41;
  a->v1.w = a41 * b12 + a42 * b22 + a43 * b32 + a44 * b42;
  a->v2.w = a41 * b13 + a42 * b23 + a43 * b33 + a44 * b43;
  a->v3.w = a41 * b14 + a42 * b24 + a43 * b34 + a44 * b44;
}

void mat4_invert(mat4 * m)
{
  double n11 = m->v0.x, n21 = m->v0.y, n31 = m->v0.z, n41 = m->v0.w,
    n12 = m->v1.x, n22 = m->v1.y, n32 = m->v1.z, n42 = m->v1.w,
    n13 = m->v2.x, n23 = m->v2.y, n33 = m->v2.z, n43 = m->v2.w,
    n14 = m->v3.x, n24 = m->v3.y, n34 = m->v3.z, n44 = m->v3.w;

  double t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
  double t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
  double t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
  double t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

  double det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;

  if (det == 0)
  {
    return;
  }

  double detInv = 1 / det;

  m->v0.x = t11 * detInv;
  m->v0.y = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * detInv;
  m->v0.z = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * detInv;
  m->v0.w = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * detInv;

  m->v1.x = t12 * detInv;
  m->v1.y = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * detInv;
  m->v1.z = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * detInv;
  m->v1.w = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * detInv;

  m->v2.x = t13 * detInv;
  m->v2.y = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * detInv;
  m->v2.z = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * detInv;
  m->v2.w = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * detInv;

  m->v3.x = t14 * detInv;
  m->v3.y = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * detInv;
  m->v3.z = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * detInv;
  m->v3.w = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * detInv;
}

void vec4_apply_mat4(vec4 * vec, const mat4 * mat)
{
  double x = vec->x, y = vec->y, z = vec->z, w = vec->w;

  vec->x = mat->v0.x * x + mat->v1.x * y + mat->v2.x * z + mat->v3.x * w;
  vec->y = mat->v0.y * x + mat->v1.y * y + mat->v2.y * z + mat->v3.y * w;
  vec->z = mat->v0.z * x + mat->v1.z * y + mat->v2.z * z + mat->v3.z * w;
  vec->w = mat->v0.w * x + mat->v1.w * y + mat->v2.w * z + mat->v3.w * w;
}

void vec3_apply_mat3(vec3 * vec, const mat3 * mat)
{
  double x = vec->x, y = vec->y, z = vec->z;

  vec->x = mat->v0.x * x + mat->v1.x * y + mat->v2.x * z;
  vec->y = mat->v0.y * x + mat->v1.y * y + mat->v2.y * z;
  vec->z = mat->v0.z * x + mat->v1.z * y + mat->v2.z * z;
}

void vec4_multiply_scalar(vec4 * vec, double scalar)
{
  vec->x *= scalar;
  vec->y *= scalar;
  vec->z *= scalar;
  vec->w *= scalar;
}

void vec4_add_scalar(vec4 * vec, double scalar)
{
  vec->x += scalar;
  vec->y += scalar;
  vec->z += scalar;
  vec->w += scalar;
}

void vec3_multiply_scalar(vec3 * vec, double scalar)
{
  vec->x *= scalar;
  vec->y *= scalar;
  vec->z *= scalar;
}

void vec3_add_scalar(vec3 * vec, double scalar)
{
  vec->x += scalar;
  vec->y += scalar;
  vec->z += scalar;
}

void mat3_invert(mat3 * m)
{
  double n11 = m->v0.x, n21 = m->v0.y, n31 = m->v0.z,
    n12 = m->v1.x, n22 = m->v1.y, n32 = m->v1.z,
    n13 = m->v2.x, n23 = m->v2.y, n33 = m->v2.z;

  double t11 = n33 * n22 - n32 * n23;
  double t12 = n32 * n13 - n33 * n12;
  double t13 = n23 * n12 - n22 * n13;

  double det = n11 * t11 + n21 * t12 + n31 * t13;

  if (det == 0)
  {
    return;
  }

  double detInv = 1 / det;

  m->v0.x = t11 * detInv;
  m->v0.y = (n31 * n23 - n33 * n21) * detInv;
  m->v0.z = (n32 * n21 - n31 * n22) * detInv;

  m->v1.x = t12 * detInv;
  m->v1.y = (n33 * n11 - n31 * n13) * detInv;
  m->v1.z = (n31 * n12 - n32 * n11) * detInv;

  m->v2.x = t13 * detInv;
  m->v2.y = (n21 * n13 - n23 * n11) * detInv;
  m->v2.z = (n22 * n11 - n21 * n12) * detInv;
}

void mat3_transpose(mat3 * m)
{
  double tmp;
  tmp = m->v0.y; m->v0.y = m->v1.x; m->v1.x = tmp;
  tmp = m->v0.z; m->v0.z = m->v2.x; m->v2.x = tmp;
  tmp = m->v1.z; m->v1.z = m->v2.y; m->v2.y = tmp;
}

void mat3_from_mat4(mat3 * out, const mat4 * mat)
{
  out->v0 = (vec3){ mat->v0.x, mat->v0.y, mat->v0.z };
  out->v1 = (vec3){ mat->v1.x, mat->v1.y, mat->v1.z };
  out->v2 = (vec3){ mat->v2.x, mat->v2.y, mat->v2.z };
}

void vec3_print(const vec3 * vec)
{
  printf("%f %f %f\n", vec->x, vec->y, vec->z);
}

void vec4_print(const vec4 * vec)
{
  printf("%f %f %f %f\n", vec->x, vec->y, vec->z, vec->w);
}

void mat3_print(const mat3 * mat)
{
  printf("%f %f %f\n", mat->v0.x, mat->v1.x, mat->v2.x);
  printf("%f %f %f\n", mat->v0.y, mat->v1.y, mat->v2.y);
  printf("%f %f %f\n", mat->v0.z, mat->v1.z, mat->v2.z);
}

void mat4_print(const mat4 * mat)
{
  printf("%f %f %f %f\n", mat->v0.x, mat->v1.x, mat->v2.x, mat->v3.x);
  printf("%f %f %f %f\n", mat->v0.y, mat->v1.y, mat->v2.y, mat->v3.y);
  printf("%f %f %f %f\n", mat->v0.z, mat->v1.z, mat->v2.z, mat->v3.z);
  printf("%f %f %f %f\n", mat->v0.w, mat->v1.w, mat->v2.w, mat->v3.w);
}

#endif
