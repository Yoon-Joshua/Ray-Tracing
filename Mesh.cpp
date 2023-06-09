#include "Mesh.h"

#include <iostream>

extern const float epsilon;

unsigned Mesh::num_faces() { return faces.size(); }

unsigned Mesh::num_normals() { return this->attr.normals.size(); }

unsigned Mesh::num_vertices() { return attr.vertices.size(); }

unsigned Mesh::num_texcoords() { return attr.texcoords.size(); }

Attribute& Mesh::get_attr() { return attr; }

std::vector<Triangle>& Mesh::get_faces() { return faces; }

void Mesh::add_face(Triangle& f) {
  f.set_attr(&attr);
  f.init();
  faces.emplace_back(f);
}

// -----------------------------------------------------------------------

void Attribute::add_vertex(Vector3D v) { vertices.emplace_back(v); }

void Attribute::add_normal(Vector3D v) { normals.emplace_back(v); }

void Attribute::add_texcoord(Vector3D v) { texcoords.emplace_back(v); }

// -----------------------------------------------------------------------

Triangle::Triangle() : attr(nullptr) {}

Triangle::~Triangle() {}

void Triangle::set_attr(Attribute* a) { this->attr = a; }

Vector3D Triangle::centre() {
  Vector3D sum(0, 0, 0);
  for (auto v : vertices) {
    sum = sum + attr->vertices[v];
  }
  sum = sum / 3;
  return sum;
}
float Triangle::intersect(Ray r) {
  Vector3D n = normal_average();
  if (Vector3D::dot(r.direction, n) >= 0) return __FLT_MIN__;
  Vector3D u = attr->vertices[vertices[1]] - attr->vertices[vertices[0]];
  Vector3D v = attr->vertices[vertices[2]] - attr->vertices[vertices[0]];
  // Third-order determinant
  float determinant =
      u.y * v.z * (-r.direction.x) + u.z * v.x * (-r.direction.y) +
      u.x * v.y * (-r.direction.z) - u.x * v.z * (-r.direction.y) -
      u.y * v.x * (-r.direction.z) - u.z * v.y * (-r.direction.x);

  if (determinant < epsilon && determinant > -epsilon) return __FLT_MIN__;
  Vector3D constant = r.origin - attr->vertices[vertices[0]];
  float alpha = constant.y * v.z * (-r.direction.x) +
                constant.z * v.x * (-r.direction.y) +
                constant.x * v.y * (-r.direction.z) -
                constant.x * v.z * (-r.direction.y) -
                constant.y * v.x * (-r.direction.z) -
                constant.z * v.y * (-r.direction.x);
  float belta = u.y * constant.z * (-r.direction.x) +
                u.z * constant.x * (-r.direction.y) +
                u.x * constant.y * (-r.direction.z) -
                u.x * constant.z * (-r.direction.y) -
                u.y * constant.x * (-r.direction.z) -
                u.z * constant.y * (-r.direction.x);
  float t = u.y * v.z * (constant.x) + u.z * v.x * (constant.y) +
            u.x * v.y * (constant.z) - u.x * v.z * (constant.y) -
            u.y * v.x * (constant.z) - u.z * v.y * (constant.x);
  alpha /= determinant;
  belta /= determinant;
  t /= determinant;
  if (t <= 0 || alpha + belta > 1 || alpha < 0 || belta < 0)
    return __FLT_MIN__;
  else
    return t;
}
Vector3D Triangle::normal_average() {
  Vector3D ret(0, 0, 0);
  for (unsigned n = 0; n < this->normals.size(); ++n) {
    ret = ret + attr->normals[normals[n]];
  }
  ret = ret / 3;
  ret.normalize();
  return ret;
}

float Triangle::area() {
  Vector3D u = attr->vertices[vertices[1]] - attr->vertices[vertices[0]];
  Vector3D v = attr->vertices[vertices[2]] - attr->vertices[vertices[0]];
  Vector3D product = Vector3D::cross(u, v);
  return product.length() / 2;
}

void Triangle::init() {
  float xmin = __FLT_MAX__;
  float ymin = __FLT_MAX__;
  float zmin = __FLT_MAX__;
  float xmax = __FLT_MIN__;
  float ymax = __FLT_MIN__;
  float zmax = __FLT_MIN__;
  center.assign(0, 0, 0);
  face_normal.assign(0, 0, 0);
  for (int i = 0; i < 3; ++i) {
    Vector3D v = attr->vertices[vertices[i]];
    center = center + v;
    face_normal = face_normal + attr->normals[normals[i]];
    if (v.x > xmax) xmax = v.x;
    if (v.x < xmin) xmin = v.x;
    if (v.y > ymax) ymax = v.y;
    if (v.y < ymin) ymin = v.y;
    if (v.z > zmax) zmax = v.z;
    if (v.z < zmin) zmin = v.z;
  }
  center = center / 3;
  face_normal = face_normal / 3;
  diagonal_vertices[0].assign(xmin, ymin, zmin);
  diagonal_vertices[1].assign(xmax, ymax, zmax);
}

Vector3D Triangle::texture_mapping(Vector3D point) {
  Vector3D u = attr->vertices[vertices[1]] - attr->vertices[vertices[0]];
  Vector3D v = attr->vertices[vertices[2]] - attr->vertices[vertices[0]];
  float d, a, b;
  d = u.x * v.y - v.x * u.y;
  if (d > epsilon || d < -epsilon) {
    a = (point.x * v.y - v.x * point.y) / d;
    b = (u.x * point.y - point.x * u.y) / d;
    goto k;
  }
  d = u.x * v.z - v.x * u.z;
  if (d > epsilon || d < -epsilon) {
    a = (point.x * v.z - v.x * point.z) / d;
    b = (u.x * point.z - point.x * u.z) / d;
    goto k;
  }
  d = u.y * v.z - v.y * u.z;
  if (d > epsilon || d < -epsilon) {
    a = (point.y * v.z - v.y * point.z) / d;
    b = (u.y * point.z - point.y * u.z) / d;
    goto k;
  }

k:
  //if (a > 1 || a < 0 || b > 1 || b < 0) std::cout << "FUCK" << std::endl;

  u = attr->texcoords[texcords[1]] - attr->texcoords[texcords[0]];
  v = attr->texcoords[texcords[2]] - attr->texcoords[texcords[0]];
  Vector3D t = a * u + b * v;

  while (t.x > 1) t.x = t.x - 1;
  while (t.x < 0) t.x = t.x + 1;
  while (t.y < 0) t.y = t.y + 1;
  while (t.y > 1) t.y = t.y - 1;

  int pixel_x = t.x * (material->map_width - 1);
  int pixel_y = t.y * (material->map_height - 1);
  float x = material->map[(material->map_width * pixel_y + pixel_x) * 3];
  float y = material->map[(material->map_width * pixel_y + pixel_x) * 3 + 1];
  float z = material->map[(material->map_width * pixel_y + pixel_x) * 3 + 2];
  return Vector3D((float)z / 255.0f, (float)y / 255.0f, (float)x / 255.0f);
}