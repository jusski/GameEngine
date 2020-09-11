internal inline bool32
IsSet(sim_entity *Entity, u32 Flag)
{
    bool32 Result = Entity->Flags & Flag;

    return(Result);
}

internal inline void
AddFlag(sim_entity *Entity, u32 Flag)
{
    Entity->Flags |= Flag;
}

internal inline void
ClearFlag(sim_entity *Entity, u32 Flag)
{
    Entity->Flags &= ~Flag;
}

