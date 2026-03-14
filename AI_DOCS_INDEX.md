# SwayFX AI Documentation Index

This repository contains comprehensive AI-first documentation to help you leverage AI tools for rapid feature development.

## Documentation Structure

### 🎯 Start Here

**[AGENTS.md](AGENTS.md)** - Primary AI development guide
- Project overview and architecture
- Tech stack and dependencies
- Codebase organization and critical files
- Coding conventions and patterns
- Development workflow and testing
- SwayFX-specific features (blur, shadows, effects)
- Common task patterns
- Domain-specific knowledge (Wayland, wlroots, compositors)

### 🏗️ Deep Technical Details

**[ARCHITECTURE.md](ARCHITECTURE.md)** - Detailed system architecture
- Core data structures (server, container, output, view)
- Subsystem deep dives (input, transaction, IPC, rendering)
- Scene graph organization
- Configuration system internals
- Effect implementation details
- Performance optimization strategies
- Code reading guide

### 📚 Code Examples

**[PATTERNS.md](PATTERNS.md)** - Concrete code patterns from actual codebase
- Command implementation examples
- Container management patterns
- Wayland listener patterns
- List management patterns
- Transaction patterns
- Scene graph patterns
- IPC and JSON serialization
- Common gotchas and solutions

### ⚡ Quick Lookup

**[QUICK_REFERENCE.md](QUICK_REFERENCE.md)** - Fast reference guide
- File location cheat sheet
- Common task templates
- Command reference (blur, shadow, IPC)
- Build and debug commands
- Common code snippets
- Git workflow
- Troubleshooting guide

### 🌐 LLM Discovery

**[llms.txt](llms.txt)** - Index for LLM tool discovery
- Brief project summary
- Links to all documentation
- Key concepts overview
- Quick reference information

## Documentation Philosophy

This documentation follows proven best practices from 2024-2026 research:

✅ **Proven patterns:**
- AGENTS.md format (60,000+ projects use this)
- Hierarchical markdown structure
- Short paragraphs for better retrieval
- Concrete examples over abstractions
- Real file references (path:line format)
- Token-efficient formatting

✅ **Multiple perspectives:**
- High-level overview (AGENTS.md)
- Deep technical details (ARCHITECTURE.md)
- Practical examples (PATTERNS.md)
- Quick lookup (QUICK_REFERENCE.md)

✅ **AI-optimized:**
- Clear section boundaries
- Searchable structure
- Context-rich descriptions
- Examples from actual codebase

## How to Use This Documentation

### For AI Assistants

1. **Start with AGENTS.md** - Get project context and conventions
2. **Reference ARCHITECTURE.md** - When working on complex features
3. **Copy from PATTERNS.md** - When implementing similar functionality
4. **Check QUICK_REFERENCE.md** - For file locations and common commands

### For Developers

1. **Read AGENTS.md** - Understand the system holistically
2. **Explore ARCHITECTURE.md** - Deep dive into subsystems
3. **Study PATTERNS.md** - Learn by example
4. **Bookmark QUICK_REFERENCE.md** - Daily development reference

## What Makes This Effective

Based on research of effective AI-first documentation:

1. **Standards-based**: Uses AGENTS.md (industry standard)
2. **Comprehensive**: Covers architecture, patterns, conventions, domain knowledge
3. **Practical**: Real code examples, not invented ones
4. **Structured**: Clear hierarchy, short sections, bullet points
5. **Maintained**: Reflects actual codebase (not outdated)
6. **Multi-format**: Different docs for different use cases

## Documentation Coverage

- ✅ Project overview and purpose
- ✅ Architecture and design patterns
- ✅ Coding conventions and style
- ✅ Core data structures
- ✅ Subsystem interactions
- ✅ Build system and dependencies
- ✅ Testing and debugging approaches
- ✅ Domain knowledge (Wayland, compositors)
- ✅ SwayFX-specific features (effects system)
- ✅ Real code examples from codebase
- ✅ Common pitfalls and solutions
- ✅ File organization and navigation
- ✅ Command reference
- ✅ Quick lookup guides

## Human-Readable Documentation

For user-facing documentation, see:
- **README.md** - Features, installation, usage
- **CONTRIBUTING.md** - Contribution guidelines
- **Man pages** - `sway(1)`, `sway(5)`, `swaymsg(1)`

## Feedback

This documentation was created using research-backed best practices for AI-first codebases. If you find areas for improvement, please open an issue or submit a PR.

## Research Sources

Documentation structure based on:
- AGENTS.md format (agents.md/)
- llms.txt standard (llmstxt.org)
- Anthropic effective context engineering
- 2024-2026 best practices research

---

**Note:** This is AI-first documentation. It's optimized for AI coding assistants to quickly understand and contribute to the codebase. For human-readable documentation, start with README.md.
