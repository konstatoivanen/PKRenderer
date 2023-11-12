#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#WithAtomicCounter

#pragma PROGRAM_COMPUTE
#include includes/Meshlets.glsl

// @TODO NOTE that for optimization purposes PK_Draw uses uint4(material, transform, global meshlet submesh, userdata) internally (this saves a redundant struct descriptor)
#define PK_Draw uint4
layout(std430, set = 3, binding = 101) readonly restrict buffer pk_Instancing_Indices { PK_Draw pk_Instancing_Indices_Data[]; };

PK_DECLARE_BUFFER(uint, pk_Meshlet_TaskDisptaches, PK_SET_SHADER);
PK_DECLARE_VARIABLE(uint, pk_Meshlet_TaskDisptachCounter, PK_SET_SHADER);

PK_DECLARE_LOCAL_CBUFFER(pk_DrawRange)
{
    uint firstInstance;
    uint instanceCount;
};

void SetDispatchDefaultsXY(uint dispatch)
{
    PK_BUFFER_DATA(pk_Meshlet_TaskDisptaches, dispatch * 3u + 1u) = 1u;
    PK_BUFFER_DATA(pk_Meshlet_TaskDisptaches, dispatch * 3u + 2u) = 1u;
}

void AddTasksToDispatch(uint dispatch, uint taskCount)
{
    atomicAdd(PK_BUFFER_DATA(pk_Meshlet_TaskDisptaches, dispatch * 3u + 0u), taskCount);
}

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main()
{
    const uint localInstanceId = gl_GlobalInvocationID.x;

    if (localInstanceId >= instanceCount)
    {
        return;
    }

    const uint instanceId = firstInstance + localInstanceId;
    const PK_Draw draw = pk_Instancing_Indices_Data[instanceId];
    const PKSubmesh sm = Meshlet_Load_Submesh(draw.z);
    const uint taskCount = (sm.meshletCount + (MAX_MESHLETS_PER_TASK - 1)) / MAX_MESHLETS_PER_TASK;
    const uint firstTaskIndex = PK_AtomicCounterAdd(taskCount);
    const uint lastTaskIndex = firstTaskIndex + taskCount;

    for (uint i = 0u; i < taskCount; ++i)
    {
        PKTasklet t;
        t.instanceId = instanceId;
        t.firstMeshlet = sm.firstMeshlet + i * MAX_MESHLETS_PER_TASK;
        t.meshletCount = min(sm.meshletCount - i * MAX_MESHLETS_PER_TASK, MAX_MESHLETS_PER_TASK);
        Meshlet_Store_Tasklet(firstTaskIndex + i, t);
    }

    const uint dispatch0 = firstTaskIndex / MAX_TASK_WORK_GROUPS;
    const uint dispatch1 = lastTaskIndex / MAX_TASK_WORK_GROUPS;
    const uint taskCount0 = min(lastTaskIndex, dispatch1 * MAX_TASK_WORK_GROUPS) - firstTaskIndex;
    const uint taskCount1 = lastTaskIndex - max(firstTaskIndex, dispatch1 * MAX_TASK_WORK_GROUPS);

    // New dispatch index, set offsets & default dimensions
    [[branch]]
    if (dispatch0 != dispatch1)
    {
        AddTasksToDispatch(dispatch0, taskCount0);
        SetDispatchDefaultsXY(dispatch1);
        atomicMax(PK_VARIABLE_DATA(pk_Meshlet_TaskDisptachCounter), dispatch1 + 1u);
    }
    
    // This cannot be trivially cleared so set it here.
    [[branch]]
    if (localInstanceId == 0u)
    {
        SetDispatchDefaultsXY(0u);
    }

    AddTasksToDispatch(dispatch1, taskCount1);
}