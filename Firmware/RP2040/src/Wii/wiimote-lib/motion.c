#include "motion.h"

#include "vector_math.h"

//units in meters
static const double screen_distance = 2;
static const double screen_width = 1.0;
static const double screen_aspect = 4.0 / 3.0;

static const double sensor_bar_y = screen_width / screen_aspect * 0.5;
static const double sensor_bar_width = 0.20;

static const double cam_aspect = 1024.0 / 768.0;
static const double cam_fov = 42.0;
static const double cam_far = 4.0;
static const double cam_near = 0.5;

static const uint16_t accelerometer_zero = 0x85 << 2;
static const uint16_t accelerometer_unit = 0x6C;

void look_at_pointer(mat4 * wiimote_mat, float pointer_x, float pointer_y)
{
  vec3 pointer_world = {
    (pointer_x - 0.5) * screen_width,
    (pointer_y - 0.5) * screen_width / screen_aspect,
    -screen_distance
  };

  vec3 dir = { pointer_world.x, pointer_world.y, pointer_world.z };
  vec3_normalize(&dir);

  vec3 up = { 0.0, 1.0, 0.0 };
  vec3 z = { -dir.x, -dir.y, -dir.z };

  vec3 x;
  vec3_cross(&x, &up, &z);
  vec3_normalize(&x);

  vec3 y;
  vec3_cross(&y, &z, &x);
  // vec3_normalize(&y);

  wiimote_mat->v0 = (vec4){ x.x, x.y, x.z, 0.0 };
  wiimote_mat->v1 = (vec4){ y.x, y.y, y.z, 0.0 };
  wiimote_mat->v2 = (vec4){ z.x, z.y, z.z, 0.0 };
  wiimote_mat->v3 = (vec4){ 0.0, 0.0, 0.0, 1.0 };
}

void make_cam_projection_mat(mat4 * proj_mat)
{
  double near = cam_near;
  double far = cam_far;

  double top = near * tan(cam_fov / 180.0 * M_PI * 0.5);
  double height = 2.0 * top;
  double width = cam_aspect * height;
  double left = -0.5 * width;

  double right = left + width;
  double bottom = top - height;

  proj_mat->v0 = (vec4){ 2.0 * near / (right - left), 0.0, 0.0, 0.0 };
  proj_mat->v1 = (vec4){ 0.0, 2.0 * near / (top - bottom), 0.0, 0.0 };
  proj_mat->v2 = (vec4){ (right + left) / (right - left), (top + bottom) / (top - bottom), -(far + near) / (far - near), -1.0 };
  proj_mat->v3 = (vec4){ 0.0, 0.0, -2.0 * far * near / (far - near), 0.0 };
}

void set_accelerometer(struct wiimote_state * state, const mat4 * wiimote_mat)
{
  vec3 accel = { 0, -1.0, 0 };
  mat3 accel_m;
  mat3_from_mat4(&accel_m, wiimote_mat);
  mat3_invert(&accel_m);
  mat3_transpose(&accel_m);
  vec3_apply_mat3(&accel, &accel_m);

  accel.x = fmax(-3.4, fmin(3.4, accel.x));
  accel.y = fmax(-3.4, fmin(3.4, accel.y));
  accel.z = fmax(-3.4, fmin(3.4, accel.z));

  //transform to wiimote's accelerometer coordinate system
  state->usr.accel_x = accelerometer_zero +
    (int)round((double)accelerometer_unit * -accel.x);
  state->usr.accel_z = accelerometer_zero +
    (int)round((double)accelerometer_unit * -accel.y);
  state->usr.accel_y = accelerometer_zero +
    (int)round((double)accelerometer_unit * accel.z);
}

void set_motionplus(struct wiimote_state * state, const mat4 * wiimote_mat)
{

}

void set_motion_state(struct wiimote_state * state, float pointer_x, float pointer_y)
{
  mat4 wiimote_mat;
  look_at_pointer(&wiimote_mat, pointer_x, pointer_y);

  mat4 view_mat = wiimote_mat;
  mat4_invert(&view_mat);

  mat4 model_mat;
  vec3 model_pos = (vec3){ 0.0, sensor_bar_y, -screen_distance };
  mat4_make_translation(&model_mat, &model_pos);

  mat4 proj_mat;
  make_cam_projection_mat(&proj_mat);

  mat4_mult(&view_mat, &model_mat);
  mat4_mult(&proj_mat, &view_mat);

  vec4 sensor_pt0 = { -sensor_bar_width * 0.5, 0.0, 0.0, 1.0 };
  vec4 sensor_pt1 = { sensor_bar_width * 0.5, 0.0, 0.0, 1.0 };

  vec4_apply_mat4(&sensor_pt0, &proj_mat);
  vec4_apply_mat4(&sensor_pt1, &proj_mat);

  vec4_multiply_scalar(&sensor_pt0, 1 / sensor_pt0.w);
  vec4_multiply_scalar(&sensor_pt1, 1 / sensor_pt1.w);

  vec4_add_scalar(&sensor_pt0, 1.0);
  vec4_multiply_scalar(&sensor_pt0, 0.5);

  vec4_add_scalar(&sensor_pt1, 1.0);
  vec4_multiply_scalar(&sensor_pt1, 0.5);

  double min_pt_size = 1.0;
  double max_pt_size = 15.0;

  reset_ir_object(&state->usr.ir_object[0]);
  reset_ir_object(&state->usr.ir_object[1]);

  if (sensor_pt0.x > 0 && sensor_pt0.x < 1 &&
    sensor_pt0.y > 0 && sensor_pt0.y < 1 &&
    sensor_pt0.z > 0 && sensor_pt0.z < 1)
  {
    state->usr.ir_object[0].x = round(sensor_pt0.x * 1023);
    state->usr.ir_object[0].y = round(sensor_pt0.y * 767);
    state->usr.ir_object[0].size = round(min_pt_size + 
      pow(1.0 - sensor_pt0.z, 2.0) * (max_pt_size - min_pt_size));
  }

  if (sensor_pt1.x > 0 && sensor_pt1.x < 1 &&
    sensor_pt1.y > 0 && sensor_pt1.y < 1 &&
    sensor_pt1.z > 0 && sensor_pt1.z < 1)
  {
    state->usr.ir_object[1].x = round(sensor_pt1.x * 1023);
    state->usr.ir_object[1].y = round(sensor_pt1.y * 767);
    state->usr.ir_object[1].size = round(min_pt_size +
      pow(1.0 - sensor_pt1.z, 2.0) * (max_pt_size - min_pt_size));
  }

  set_accelerometer(state, &wiimote_mat);
}