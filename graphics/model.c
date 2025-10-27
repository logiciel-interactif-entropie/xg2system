#include "graphics/model.h"

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <stdint.h>

#include "assimp/vector3.h"
#include "bgfx/c99/bgfx.h"
#include "bgfx/defines.h"
#include "cglm/cglm.h"
#include "core/log.h"
#include "core/mem.h"
#include "engine/resource.h"
#include "engine/runtime.h"
#include "engine/transform.h"
#include "flecs.h"
#include "flecs/addons/flecs_c.h"
#include "glib.h"
#include "graphics/resource.h"
#include "graphics/shader_program.h"

ECS_COMPONENT_DECLARE(model_t);
ECS_COMPONENT_DECLARE(model_group_t);
ECS_COMPONENT_DECLARE(model_instance_t);

enum vertex_layout_id {
  vli_regular_layout,
  vli_skinned_layout,
  vli_layout_count
};

struct vertex_layout layout_types[vli_layout_count] = {};

struct __attribute__((packed)) model_vertex {
  vec3 position;
  vec3 normal;
  vec4 color0;
  vec4 color1;
  vec2 texcoord;
};

static void __initialize_vli(enum vertex_layout_id id) {
  struct vertex_layout* layout = &layout_types[id];
  if (layout->initialized) return;
  layout->initialized = true;
  bgfx_vertex_layout_begin(&layout->vertex_layout_alloc,
                           BGFX_RENDERER_TYPE_NOOP);
  bgfx_vertex_layout_add(&layout->vertex_layout_alloc, BGFX_ATTRIB_POSITION, 3,
                         BGFX_ATTRIB_TYPE_FLOAT, false, false);
  bgfx_vertex_layout_add(&layout->vertex_layout_alloc, BGFX_ATTRIB_NORMAL, 3,
                         BGFX_ATTRIB_TYPE_FLOAT, false, false);
  bgfx_vertex_layout_add(&layout->vertex_layout_alloc, BGFX_ATTRIB_COLOR0, 4,
                         BGFX_ATTRIB_TYPE_FLOAT, false, false);
  bgfx_vertex_layout_add(&layout->vertex_layout_alloc, BGFX_ATTRIB_COLOR1, 4,
                         BGFX_ATTRIB_TYPE_FLOAT, false, false);
  bgfx_vertex_layout_add(&layout->vertex_layout_alloc, BGFX_ATTRIB_TEXCOORD0, 2,
                         BGFX_ATTRIB_TYPE_FLOAT, false, false);
  switch (id) {
    case vli_skinned_layout:
      break;
    default:
      break;
  }
  bgfx_vertex_layout_end(&layout->vertex_layout_alloc);
  layout->vertex_layout =
      bgfx_create_vertex_layout(&layout->vertex_layout_alloc);
}

static void __model_group_render(ecs_iter_t* it) {
  model_group_t* _group = ecs_field(it, model_group_t, 0);
  shader_t* _shader = ecs_field(it, shader_t, 1);
  for (int i = 0; i < it->count; i++) {
    model_group_t* group = &_group[i];
    shader_t* shader = &_shader[i];

    if (!group->resource) continue;
    if (!shader->shader) continue;

    struct model_resource* model = resource_get_model(group->resource);
    if (!model->model_loaded) continue;
    struct shader_program_resource* shader_resource =
        resource_get_shader_program(shader->shader);
    if (!shader_resource->program_loaded) continue;

    ecs_query_t* members = ecs_query(
        it->world,
        {.terms = {{ecs_id(model_instance_t)}, {ecs_id(transform3d_t)}}});
    ecs_iter_t members_it = ecs_query_iter(it->world, members);
    int num_members = members_it.count;

    bgfx_encoder_t* encoder = bgfx_encoder_begin(false);
    while (ecs_query_next(&members_it)) {
      model_instance_t* instance = ecs_field(&members_it, model_instance_t, 0);
      transform3d_t* transform = ecs_field(&members_it, transform3d_t, 1);

      int num_attached = 0;
      for (int j = 0; j < members_it.count; j++)
        if (instance[j].group_id == it->entities[i]) num_attached++;
      if (num_attached == 0) continue;

      bgfx_instance_data_buffer_t db;
      int attached_quota =
          bgfx_get_avail_instance_data_buffer(num_attached, sizeof(mat4));
      bgfx_alloc_instance_data_buffer(&db, attached_quota, sizeof(mat4));
      if (attached_quota < num_attached) {
        LOG(ll_warn,
            "attached_quota (%i quota, %i wanted) reached, stopping (some "
            "meshes "
            "wont render)",
            attached_quota, num_attached);
      }
      mat4* data = (mat4*)db.data;
      for (int j = 0; j < members_it.count; j++) {
        if (attached_quota == 0) {
          j = members_it.count + 1;
        } else if (instance[j].group_id == it->entities[i]) {
          transform_mat4(&transform[j], data[j]);
          attached_quota--;
        }
      }

      struct mesh* mesh = g_array_index(model->meshes, struct mesh*, 0);
      bgfx_encoder_set_vertex_buffer(encoder, 0, mesh->vertex_buffer, 0,
                                     mesh->num_vertices);
      bgfx_encoder_set_index_buffer(encoder, mesh->index_buffer, 0,
                                    mesh->num_indices);
      bgfx_encoder_set_instance_data_buffer(encoder, &db, 0, num_attached);
      bgfx_encoder_submit(encoder, 0, shader_resource->program, 0,
                          BGFX_DISCARD_ALL);
    }
    bgfx_encoder_end(encoder);
  }
}

static void __model_render(ecs_iter_t* it) {
  model_t* _model = ecs_field(it, model_t, 0);
  transform3d_t* _transform = ecs_field(it, transform3d_t, 1);
  shader_t* _shader = ecs_field(it, shader_t, 2);

  bgfx_encoder_t* encoder = bgfx_encoder_begin(false);
  uint64_t state = BGFX_STATE_WRITE_R | BGFX_STATE_WRITE_G |
                   BGFX_STATE_WRITE_B | BGFX_STATE_WRITE_A |
                   BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS |
                   BGFX_STATE_CULL_CW | BGFX_STATE_MSAA;
  bgfx_encoder_set_state(encoder, state, 0);
  struct mesh* last_mesh = NULL;
  for (int i = 0; i < it->count; i++) {
    model_t* model = &_model[i];
    transform3d_t* transform = &_transform[i];
    shader_t* shader = &_shader[i];

    if (!model->resource) continue;
    struct model_resource* model_resource = resource_get_model(model->resource);
    if (!model_resource->model_loaded) continue;
    if (!shader->shader) continue;
    struct shader_program_resource* shader_program_resource =
        resource_get_shader_program(shader->shader);
    if (!shader_program_resource->program_loaded) {
      LOG(ll_warn, "Shader resource %s not loaded yet",
          shader->shader->resource_name);
      continue;
    }

    mat4 matrix;
    transform_mat4(transform, matrix);
    bgfx_encoder_set_transform(encoder, matrix, 1);

    for (int i = 0; i < model_resource->meshes->len; i++) {
      struct mesh* mesh =
          g_array_index(model_resource->meshes, struct mesh*, i);
      bgfx_encoder_set_index_buffer(encoder, mesh->index_buffer, 0,
                                    mesh->num_indices);
      bgfx_encoder_set_vertex_buffer(encoder, 0, mesh->vertex_buffer, 0,
                                     mesh->num_vertices);
      bgfx_encoder_submit(encoder, 0, shader_program_resource->program, 0.f,
                          BGFX_DISCARD_ALL ^ BGFX_DISCARD_TRANSFORM);
    }
    bgfx_encoder_discard(encoder, BGFX_DISCARD_TRANSFORM);
  }
  bgfx_encoder_end(encoder);
}

static void __model_free(ecs_iter_t* it) {
  model_t* model = ecs_field(it, model_t, 0);
  resource_unref(model->resource);
}
static void __model_group_free(ecs_iter_t* it) {
  model_group_t* model = ecs_field(it, model_group_t, 0);
  resource_unref(model->resource);
}

void runtime_register_model(struct runtime* runtime) {
  ECS_COMPONENT_DEFINE(runtime->ecs, model_t);
  ECS_COMPONENT_DEFINE(runtime->ecs, model_group_t);
  ECS_COMPONENT_DEFINE(runtime->ecs, model_instance_t);

  ECS_SYSTEM(runtime->ecs, __model_render, EcsPostUpdate, model_t,
             transform3d_t, shader_t);
  ECS_SYSTEM(runtime->ecs, __model_group_render, EcsPostUpdate, model_group_t,
             shader_t);
  ECS_OBSERVER(runtime->ecs, __model_free, EcsOnRemove, model_t);
  ECS_OBSERVER(runtime->ecs, __model_group_free, EcsOnRemove, model_group_t);
}

static void __mesh_process(const struct aiScene* scene, struct aiMesh* mesh,
                           struct mesh* output) {
  GArray* mesh_vertices = g_array_sized_new(
      false, true, sizeof(struct model_vertex), mesh->mNumVertices);
  for (int i = 0; i < mesh->mNumVertices; i++) {
    struct model_vertex vertex;

    struct aiVector3D position = mesh->mVertices[i];
    vertex.position[0] = position.x;
    vertex.position[1] = position.y;
    vertex.position[2] = position.z;

    if (mesh->mNormals) {
      struct aiVector3D normal = mesh->mNormals[i];
      vertex.normal[0] = normal.x;
      vertex.normal[1] = normal.y;
      vertex.normal[2] = normal.z;
    }

    if (mesh->mTextureCoords[0]) {
      vertex.texcoord[0] = mesh->mTextureCoords[0][i].x;
      vertex.texcoord[1] = mesh->mTextureCoords[0][i].y;
    }

    if (mesh->mColors[0]) {
      vertex.color0[0] = mesh->mColors[0][i].r;
      vertex.color0[1] = mesh->mColors[0][i].g;
      vertex.color0[2] = mesh->mColors[0][i].b;
      vertex.color0[3] = mesh->mColors[0][i].a;
    } else {
      vertex.color0[0] = 1.f;
      vertex.color0[1] = 1.f;
      vertex.color0[2] = 1.f;
      vertex.color0[3] = 1.f;
    }

    if (mesh->mColors[1]) {
      vertex.color1[0] = mesh->mColors[1][i].r;
      vertex.color1[1] = mesh->mColors[1][i].g;
      vertex.color1[2] = mesh->mColors[1][i].b;
      vertex.color1[3] = mesh->mColors[1][i].a;
    }

    g_array_append_val(mesh_vertices, vertex);
  }

  __initialize_vli(vli_regular_layout);

  output->selected_vertex_layout = &layout_types[vli_regular_layout];
  output->num_vertices = mesh_vertices->len;
  output->vertex_buffer = bgfx_create_vertex_buffer(
      bgfx_copy(mesh_vertices->data,
                mesh_vertices->len * sizeof(struct model_vertex)),
      &output->selected_vertex_layout->vertex_layout_alloc, BGFX_BUFFER_NONE);

  GArray* mesh_indices = g_array_new(false, true, sizeof(unsigned int));
  for (int i = 0; i < mesh->mNumFaces; i++) {
    struct aiFace* face = &mesh->mFaces[i];
    for (int j = 0; j < face->mNumIndices; j++) {
      g_array_append_val(mesh_indices, face->mIndices[j]);
    }
  }

  output->num_indices = mesh_indices->len;
  output->index_buffer = bgfx_create_index_buffer(
      bgfx_copy(mesh_indices->data, mesh_indices->len * sizeof(unsigned int)),
      BGFX_BUFFER_INDEX32);

  LOG(ll_debug, "Created mesh %p with %i vertices, %i indices", output,
      output->num_vertices, output->num_indices);

  g_array_free(mesh_vertices, true);
  g_array_free(mesh_indices, true);
}

static void __scene_process(const struct aiScene* scene,
                            struct model_resource* model) {
  for (int i = 0; i < scene->mNumMeshes; i++) {
    struct mesh* mesh = HEAP_ALLOC_TYPE(struct mesh);
    __mesh_process(scene, scene->mMeshes[i], mesh);
    g_array_append_val(model->meshes, mesh);
  }
}

static void __resource_graphics_ready(struct resource* resource) {
  struct graphics_resource* graphics_resource = resource_get_graphics(resource);
  struct model_resource* model_resource = resource_get_model(resource);

  const struct aiScene* scene = aiImportFileFromMemory(
      graphics_resource->loaded_graphics_data->data,
      graphics_resource->loaded_graphics_data->size,
      aiProcess_CalcTangentSpace | aiProcess_Triangulate |
          aiProcess_JoinIdenticalVertices | aiProcess_SortByPType,
      "");
  if (!scene) {
    LOG(ll_error, "Scene failed to import %s", aiGetErrorString());
    goto free_exit;
  }

  __scene_process(scene, model_resource);
  model_resource->model_loaded = true;

  aiReleaseImport(scene);
free_exit:
  HEAP_FREE(graphics_resource->loaded_graphics_data);
}

static void __resource_graphics_free(struct resource* resource) {
  struct graphics_resource* graphics_resource = resource_get_graphics(resource);
  struct model_resource* model_resource = resource_get_model(resource);
  if (model_resource->model_loaded) {
    for (int i = 0; i < model_resource->meshes->len; i++) {
      struct mesh* mesh =
          g_array_index(model_resource->meshes, struct mesh*, i);
      mesh->selected_vertex_layout->references--;
      if (mesh->selected_vertex_layout->references == 0) {
        mesh->selected_vertex_layout->initialized = false;
        bgfx_destroy_vertex_layout(mesh->selected_vertex_layout->vertex_layout);
      }

      bgfx_destroy_vertex_buffer(mesh->vertex_buffer);
      bgfx_destroy_index_buffer(mesh->index_buffer);

      HEAP_FREE(mesh);
    }
  }

  HEAP_FREE(model_resource);
}

void resource_init_model(struct resource* resource) {
  resource_init_graphics(resource);
  struct graphics_resource* graphics_resource = resource_get_graphics(resource);
  graphics_resource->userdata = HEAP_ALLOC_TYPE(struct model_resource);
  graphics_resource->on_graphics_ready = __resource_graphics_ready;
  graphics_resource->free_data = __resource_graphics_free;

  resource_get_model(resource)->meshes =
      g_array_new(false, true, sizeof(struct mesh*));
  resource_get_model(resource)->model_loaded = false;
}

struct model_resource* resource_get_model(struct resource* resource) {
  struct graphics_resource* graphics_resource = resource_get_graphics(resource);
  return (struct model_resource*)graphics_resource->userdata;
}
