#pragma once

// Assumes nvidia subgroup size 32

uint LinearSwapId(uint id) 
{ 
    return (id & 0xFEu) + (~id & 0x1u); 
}

uint QuadSwapIdHorizontal(uint id) 
{ 
    return LinearSwapId(id); 
}

uint QuadSwapIdVertical16x2(uint id) 
{ 
    return (id + 16u) & 31u; 
}

uint QuadSwapIdDiagonal16x2(uint id) 
{ 
    return LinearSwapId((id + 16u) & 31u); 
}

uint QuadSwapIdVertical8x8(uint id) 
{ 
    return (LinearSwapId(id >> 3u) << 3u) + (id & 7u);
}

uint QuadSwapIdDiagonal8x8(uint id) 
{ 
    uint base = LinearSwapId(id >> 3u) << 3u;
    return base + LinearSwapId(id & 7u);
}
