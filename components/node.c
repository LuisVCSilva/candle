#include "node.h"
#include "name.h"
#include "spacial.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

DEC_CT(ct_node);

static void c_node_init(c_node_t *self)
{
	self->children = NULL;
	self->children_size = 0;
	self->model = mat4();
	self->cached = 0;
	self->parent = entity_null;
}

c_node_t *c_node_new()
{
	c_node_t *self = component_new(ct_node);
	return self;
}

static int c_node_changed(c_node_t *self)
{
	ulong i;
	self->cached = 0;
	for(i = 0; i < self->children_size; i++)
	{
		c_node_changed(c_node(&self->children[i]));
	}
	return 1;
}

entity_t c_node_get_by_name(c_node_t *self, const char *name)
{
	ulong i;
	for(i = 0; i < self->children_size; i++)
	{
		entity_t child = self->children[i];
		c_name_t *child_name = c_name(&child);
		c_node_t *child_node;

		if(!strncmp(child_name->name, name, sizeof(child_name->name) - 1))
		{
			return child;
		}

		child_node = c_node(&child);
		if(child_node)
		{
			entity_t response = c_node_get_by_name(child_node, name);
			if(response != entity_null)
			{
				return response;
			}
		}
	}
	return entity_null;
}

void c_node_add(c_node_t *self, int num, ...)
{
	va_list children;

	ulong i = self->children_size;
	self->children_size += num;
	self->children = realloc(self->children,
			(sizeof *self->children) * self->children_size);


    va_start(children, num);

	for(; i < self->children_size; i++)
	{
		entity_t child = va_arg(children, entity_t);

		self->children[i] = child;
		c_node_t *child_node = c_node(&child);
		if(!child_node)
		{
			entity_add_component(child, (c_t*)c_node_new());
			child_node = c_node(&child);
		}
		child_node->parent = self->super.entity;
		c_node_changed(child_node);
	}

	va_end(children);
}

void c_node_register()
{
	ct_t *ct = ct_new("c_node", &ct_node, sizeof(c_node_t),
			(init_cb)c_node_init, 1, ct_spacial);

	ct_listener(ct, ENTITY, spacial_changed, c_node_changed);
}

void c_node_update_model(c_node_t *self)
{
	if(self->cached) return;
	self->cached = 1;

	entity_t parent = self->parent;

	if(self->parent != entity_null)
	{
		c_node_t *parent_node = c_node(&parent);
		c_node_update_model(parent_node);

		self->model = mat4_mul(parent_node->model,
				c_spacial(self)->model_matrix);
	}
	else
	{
		/* self->model = mat4(); */
		self->model = c_spacial(self)->model_matrix;
	}
}

vec3_t c_node_global_to_local(c_node_t *self, vec3_t vec)
{
	mat4_t inv;
	c_node_update_model(self);
	inv = mat4_invert(self->model);
	return mat4_mul_vec4(inv, vec4(vec.x, vec.y, vec.z, 1.0)).xyz;
}
