# MicroJSON
Simple, Extremely Fast, Low level and lightweight json parser/generator
written in c89

# Features
* Simple
* Embeddable (No need build systems)
* Self Contained (No external library aside from libc)

# Optimizations Techniques
* SIMD Instructions (ARM NEON)
* Aggressive Loop Unrolling
* Memory Aligned allocator
* Cache friendly Array Based Hash
* Tag Unions for multiple types

