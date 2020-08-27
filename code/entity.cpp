inline bool32
IsSet(sim_entity *Entity, u32 Flag)
{
    bool32 Result = Entity->Flags & Flag;

    return(Result);
}

inline void
AddFlag(sim_entity *Entity, u32 Flag)
{
    Entity->Flags |= Flag;
}

inline void
ClearFlag(sim_entity *Entity, u32 Flag)
{
    Entity->Flags &= ~Flag;
}

