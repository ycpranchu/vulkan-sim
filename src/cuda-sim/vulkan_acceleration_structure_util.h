#include "vulkan/vulkan.h"
#include "vulkan/vulkan_intel.h"

#include "vulkan/anv_acceleration_structure.h"
#include "vulkan/anv_public.h"

static uint8_t *
get_anv_accel_address(VkAccelerationStructureKHR AS)
{
    uint8_t *addr = ((uint8_t *)AS) + 48;
    return (uint8_t *)anv_address_map(*((struct anv_address *)addr));
}


#define GEN_RT_BVH_VEC3_length                 3
struct GEN_RT_BVH_VEC3 {
   float                                X;
   float                                Y;
   float                                Z;
};

static uint8_t *
GEN_RT_BVH_VEC3_unpack(struct GEN_RT_BVH_VEC3* dst,
                              uint8_t *data)
{
   dst->X = *((float *)data);
   data += sizeof(float);

   dst->Y = *((float *)data);
   data += sizeof(float);

   dst->Y = *((float *)data);
   data += sizeof(float);

   return data;
}

#define GEN_RT_BVH_length                     16
struct GEN_RT_BVH {
   uint64_t                             RootNodeOffset;
   struct GEN_RT_BVH_VEC3               BoundsMin;
   struct GEN_RT_BVH_VEC3               BoundsMax;
};

static uint8_t *
GEN_RT_BVH_unpack(struct GEN_RT_BVH* dst,
                              uint8_t *data)
{
   dst->RootNodeOffset = *((uint64_t *)data);
   data += sizeof(uint64_t);

   data = GEN_RT_BVH_VEC3_unpack(&(dst->BoundsMin), data);
   data = GEN_RT_BVH_VEC3_unpack(&(dst->BoundsMax), data);

   return data;
}

#define GEN_RT_BVH_INTERNAL_NODE_length       16
struct GEN_RT_BVH_INTERNAL_NODE {
   struct GEN_RT_BVH_VEC3               Origin;
   int32_t                              ChildOffset;
   uint32_t                             NodeType;
#define NODE_TYPE_INTERNAL                       0
#define NODE_TYPE_INSTANCE                       1
#define NODE_TYPE_PROCEDURAL                     3
#define NODE_TYPE_QUAD                           4
#define NODE_TYPE_INVALID                        7
   int32_t                              ChildBoundsExponentX;
   int32_t                              ChildBoundsExponentY;
   int32_t                              ChildBoundsExponentZ;
   uint32_t                             NodeRayMask;
   uint32_t                             ChildSize[6];
   uint32_t                             ChildType[6];
   uint32_t                             StartPrimitive[6];
   uint32_t                             ChildLowerXBound[6];
   uint32_t                             ChildUpperXBound[6];
   uint32_t                             ChildLowerYBound[6];
   uint32_t                             ChildUpperYBound[6];
   uint32_t                             ChildLowerZBound[6];
   uint32_t                             ChildUpperZBound[6];
};

static uint8_t *
GEN_RT_BVH_INTERNAL_NODE_unpack(struct GEN_RT_BVH_INTERNAL_NODE* dst,
                              uint8_t *data)
{
   data = GEN_RT_BVH_VEC3_unpack(&(dst->Origin), data);

   dst->ChildOffset = *((int32_t *)data);
   data += sizeof(int32_t);

   dst->NodeType = (uint32_t)(*data);
   data += 1;

   data += 1; //one unused byte

   dst->ChildBoundsExponentX = (int32_t)(*data);
   data += 1;

   dst->ChildBoundsExponentY = (int32_t)(*data);
   data += 1;

   dst->ChildBoundsExponentZ = (int32_t)(*data);
   data += 1;

   dst->NodeRayMask = (uint32_t)(*data);
   data += 1;

   for(int i = 0; i < 6; i++)
   {
      uint8_t temp = (*data) & (0x3f);
      data += 1;

      dst->ChildSize[i] = temp & (0x03);
      dst->ChildType[i] = temp >> 2;
      dst->StartPrimitive[i] = temp >> 2;
   }

   for(int i = 0; i < 6; i++)
   {
      dst->ChildLowerXBound[i] = (uint32_t)(*data);
      data += 1;
   }

   for(int i = 0; i < 6; i++)
   {
      dst->ChildUpperXBound[i] = (uint32_t)(*data);
      data += 1;
   }

   for(int i = 0; i < 6; i++)
   {
      dst->ChildLowerYBound[i] = (uint32_t)(*data);
      data += 1;
   }

   for(int i = 0; i < 6; i++)
   {
      dst->ChildUpperYBound[i] = (uint32_t)(*data);
      data += 1;
   }

   for(int i = 0; i < 6; i++)
   {
      dst->ChildLowerZBound[i] = (uint32_t)(*data);
      data += 1;
   }

   for(int i = 0; i < 6; i++)
   {
      dst->ChildUpperZBound[i] = (uint32_t)(*data);
      data += 1;
   }

   return data;
}


#define GEN_RT_BVH_INSTANCE_LEAF_length       32
struct GEN_RT_BVH_INSTANCE_LEAF {
   uint32_t                             ShaderIndex;
   uint32_t                             GeometryRayMask;
   uint32_t                             InstanceContributionToHitGroupIndex;
   uint32_t                             LeafType;
#define TYPE_OPAQUE_CULLING_ENABLED              0
#define TYPE_OPAQUE_CULLING_DISABLED             1
   uint32_t                             GeometryFlags;
#define GEOMETRY_OPAQUE                          1
   uint64_t                             StartNodeAddress; // address
   uint32_t                             InstanceFlags;
#define TRIANGLE_CULL_DISABLE                    1
#define TRIANGLE_FRONT_COUNTERCLOCKWISE          2
#define FORCE_OPAQUE                             4
#define FORCE_NON_OPAQUE                         8
   float                                WorldToObjectm00;
   float                                WorldToObjectm01;
   float                                WorldToObjectm02;
   float                                WorldToObjectm10;
   float                                WorldToObjectm11;
   float                                WorldToObjectm12;
   float                                WorldToObjectm20;
   float                                WorldToObjectm21;
   float                                WorldToObjectm22;
   float                                ObjectToWorldm30;
   float                                ObjectToWorldm31;
   float                                ObjectToWorldm32;
   uint64_t                             BVHAddress; // address
   uint32_t                             InstanceID;
   uint32_t                             InstanceIndex;
   float                                ObjectToWorldm00;
   float                                ObjectToWorldm01;
   float                                ObjectToWorldm02;
   float                                ObjectToWorldm10;
   float                                ObjectToWorldm11;
   float                                ObjectToWorldm12;
   float                                ObjectToWorldm20;
   float                                ObjectToWorldm21;
   float                                ObjectToWorldm22;
   float                                WorldToObjectm30;
   float                                WorldToObjectm31;
   float                                WorldToObjectm32;
};

static uint8_t *
GEN_RT_BVH_INSTANCE_LEAF_unpack(struct GEN_RT_BVH_INSTANCE_LEAF* dst,
                              uint8_t *data)
{
    dst->ShaderIndex = *((uint32_t *)data) & 0x00ffffff;
    data += 3;

    dst->GeometryRayMask = (uint32_t)(*data);
    data += 1;

    dst->InstanceContributionToHitGroupIndex = *((uint32_t *)data) & 0x00ffffff;
    data += 3;

    {
        uint8_t temp = *data >> 5;
        data += 1;

        dst->LeafType = temp & 0x01;
        dst->GeometryFlags = temp >> 1;
    }

    dst->StartNodeAddress = *((uint64_t *)data) & (1 << 48 - 1);
    data += 6;

    dst->InstanceFlags = (uint32_t)(*data);
    data += 1;

    data += 1; // one unused byte

    {
      dst->WorldToObjectm00 = *((float *)data);
      data += sizeof(float);

      dst->WorldToObjectm01 = *((float *)data);
      data += sizeof(float);
      
      dst->WorldToObjectm02 = *((float *)data);
      data += sizeof(float);

      dst->WorldToObjectm10 = *((float *)data);
      data += sizeof(float);

      dst->WorldToObjectm11 = *((float *)data);
      data += sizeof(float);

      dst->WorldToObjectm12 = *((float *)data);
      data += sizeof(float);

      dst->WorldToObjectm20 = *((float *)data);
      data += sizeof(float);

      dst->WorldToObjectm21 = *((float *)data);
      data += sizeof(float);
      
      dst->WorldToObjectm22 = *((float *)data);
      data += sizeof(float);

      dst->WorldToObjectm30 = *((float *)data);
      data += sizeof(float);

      dst->WorldToObjectm31 = *((float *)data);
      data += sizeof(float);

      dst->WorldToObjectm32 = *((float *)data);
      data += sizeof(float);
    }

    dst->BVHAddress = *((uint64_t *)data) & (1 << 48 - 1);
    data += 6;

    dst->InstanceID = *((uint32_t *)data);
    data += 4;

    dst->InstanceIndex = *((uint32_t *)data);
    data += 4;

    {
      dst->ObjectToWorldm00 = *((float *)data);
      data += sizeof(float);

      dst->ObjectToWorldm01 = *((float *)data);
      data += sizeof(float);
      
      dst->ObjectToWorldm02 = *((float *)data);
      data += sizeof(float);

      dst->ObjectToWorldm10 = *((float *)data);
      data += sizeof(float);

      dst->ObjectToWorldm11 = *((float *)data);
      data += sizeof(float);

      dst->ObjectToWorldm12 = *((float *)data);
      data += sizeof(float);

      dst->ObjectToWorldm20 = *((float *)data);
      data += sizeof(float);

      dst->ObjectToWorldm21 = *((float *)data);
      data += sizeof(float);
      
      dst->ObjectToWorldm22 = *((float *)data);
      data += sizeof(float);

      dst->ObjectToWorldm30 = *((float *)data);
      data += sizeof(float);

      dst->ObjectToWorldm31 = *((float *)data);
      data += sizeof(float);

      dst->ObjectToWorldm32 = *((float *)data);
      data += sizeof(float);
    }

    return data;
}


#define GEN_RT_BVH_PRIMITIVE_LEAF_DESCRIPTOR_length      2
struct GEN_RT_BVH_PRIMITIVE_LEAF_DESCRIPTOR {
   uint32_t                             ShaderIndex;
   uint32_t                             GeometryRayMask;
   uint32_t                             GeometryIndex;
   uint32_t                             LeafType;
#define TYPE_QUAD                                0
#define TYPE_OPAQUE_CULLING_ENABLED              0
#define TYPE_OPAQUE_CULLING_DISABLED             1
   uint32_t                             GeometryFlags;
#define GEOMETRY_OPAQUE                          1
};

static uint8_t *
GEN_RT_BVH_PRIMITIVE_LEAF_DESCRIPTOR_unpack(struct GEN_RT_BVH_PRIMITIVE_LEAF_DESCRIPTOR* dst,
                              uint8_t *data)
{
   dst->ShaderIndex = *((uint32_t *)data) & 0x00ffffff;
   data += 3;

   dst->GeometryRayMask = (uint32_t)(*data);
   data += 1;

   {
      uint32_t temp = *((uint32_t *)data);
      data += 4;

      dst->ShaderIndex = temp & 0xcfffffff;
      dst->LeafType = (temp >> 29) & 0x00000001;
      dst->GeometryFlags = (temp >> 30);
   }

   return data;
}



#define GEN_RT_BVH_QUAD_LEAF_length           16
struct GEN_RT_BVH_QUAD_LEAF {
   struct GEN_RT_BVH_PRIMITIVE_LEAF_DESCRIPTOR LeafDescriptor;
   uint32_t                             PrimitiveIndex0;
   uint32_t                             PrimitiveIndex1Delta;
   uint32_t                             j0;
   uint32_t                             j1;
   uint32_t                             j2;
   bool                                 LastQuad;
   struct GEN_RT_BVH_VEC3               QuadVertex[4];
};

static uint8_t *
GEN_RT_BVH_QUAD_LEAF_unpack(struct GEN_RT_BVH_QUAD_LEAF* dst,
                              uint8_t *data)
{
   data = GEN_RT_BVH_PRIMITIVE_LEAF_DESCRIPTOR_unpack(&(dst->LeafDescriptor), data);

   dst->PrimitiveIndex0 = *((uint32_t *)data);
   data += 4;

   {
      uint32_t temp = *((uint32_t *)data);
      data += 4;

      dst->PrimitiveIndex1Delta = temp & 0x0001ffff;
      dst->j0 = (temp >> 17) & 0x00000003;
      dst->j1 = (temp >> 19) & 0x00000003;
      dst->j2 = (temp >> 21) & 0x00000003;
   }

   data = GEN_RT_BVH_VEC3_unpack(&(dst->QuadVertex[0]), data);

   data = GEN_RT_BVH_VEC3_unpack(&(dst->QuadVertex[1]), data);

   data = GEN_RT_BVH_VEC3_unpack(&(dst->QuadVertex[2]), data);

   data = GEN_RT_BVH_VEC3_unpack(&(dst->QuadVertex[3]), data);

   return data;
}




// struct RT_BVH_VEC3 {
//    float                                X;
//    float                                Y;
//    float                                Z;
// };

// struct RT_BVH_INTERNAL_NODE {
//    struct RT_BVH_VEC3               Origin;
//    int32_t                              ChildOffset;
//    uint8_t                             NodeType;
// #define NODE_TYPE_INTERNAL                       0
// #define NODE_TYPE_INSTANCE                       1
// #define NODE_TYPE_PROCEDURAL                     3
// #define NODE_TYPE_QUAD                           4
// #define NODE_TYPE_INVALID                        7
//    int8_t                              ChildBoundsExponentX;
//    int8_t                              ChildBoundsExponentY;
//    int8_t                              ChildBoundsExponentZ;
//    uint8_t                             NodeRayMask;
//    uint8_t                             childInfo[6]; //ChildSize 0-1, ChildType 2-5, StartPrimitive 2-5
//    uint8_t                             ChildLowerXBound[6];
//    uint8_t                             ChildUpperXBound[6];
//    uint8_t                             ChildLowerYBound[6];
//    uint8_t                             ChildUpperYBound[6];
//    uint8_t                             ChildLowerZBound[6];
//    uint8_t                             ChildUpperZBound[6];
// };

// struct GEN_RT_BVH {
//    uint64_t                             RootNodeOffset;
//    struct GEN_RT_BVH_VEC3               BoundsMin;
//    struct GEN_RT_BVH_VEC3               BoundsMax;
// };

// struct GEN_RT_BVH_PRIMITIVE_LEAF_DESCRIPTOR {
//    uint32_t                             ShaderIndex;
//    uint32_t                             GeometryRayMask;
//    uint32_t                             GeometryIndex;
//    uint32_t                             LeafType;
// #define TYPE_QUAD                                0
// #define TYPE_OPAQUE_CULLING_ENABLED              0
// #define TYPE_OPAQUE_CULLING_DISABLED             1
//    uint32_t                             GeometryFlags;
// #define GEOMETRY_OPAQUE                          1
// };