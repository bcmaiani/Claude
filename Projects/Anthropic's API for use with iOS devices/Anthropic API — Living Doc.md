# Anthropic API — Living Doc

> Last updated: 2026-06-19

---

## Overview

The Claude API is a RESTful API at `https://api.anthropic.com` that provides programmatic access to Claude models and Claude Managed Agents. There are two broad modes of use:

- **Messages** — stateless, direct model calls
- **Managed Agents** — stateful, sandboxed, multi-step agent execution

---

## Available APIs

### Generally Available

| API | Endpoint | Description |
|-----|----------|-------------|
| Messages | `POST /v1/messages` | Core conversational API |
| Message Batches | `POST /v1/messages/batches` | Async bulk processing, 50% cost reduction |
| Token Counting | `POST /v1/messages/count_tokens` | Pre-flight token count before sending |
| Models | `GET /v1/models` | List available Claude models |

### Beta (opt-in via `anthropic-beta` header)

| API | Endpoint | Description |
|-----|----------|-------------|
| Files | `/v1/files` | Upload and reuse files across API calls |
| Skills | `/v1/skills` | Create custom agent skills |
| Agents | `/v1/agents` | Define versioned, reusable agent configs |
| Sessions | `/v1/sessions` | Run stateful agents in managed cloud sandboxes |
| Environments | `/v1/environments` | Configure sandbox templates for agent sessions |

Agents + Sessions + Environments together form **Claude Managed Agents**.

---

## Authentication

Every request requires:

| Header | Value | Required |
|--------|-------|----------|
| `x-api-key` | API key from Console | One of these two |
| `Authorization` | `Bearer <token>` (Workload Identity Federation) | One of these two |
| `anthropic-version` | e.g. `2023-06-01` | Yes |
| `content-type` | `application/json` | Yes |

API keys are managed at [platform.claude.com/settings/keys](https://platform.claude.com/settings/keys).

---

## Models (as of June 2026)

| Model | API String |
|-------|-----------|
| Claude Opus 4.8 | `claude-opus-4-8` |
| Claude Sonnet 4.6 | `claude-sonnet-4-6` |
| Claude Haiku 4.5 | `claude-haiku-4-5-20251001` |

Dateless model IDs (e.g. `claude-opus-4-8`) are pinned snapshots — the model behind the ID doesn't change.

---

## Request Size Limits

| Endpoint | Max size |
|----------|----------|
| Messages, Token Counting | 32 MB |
| Message Batches | 256 MB |
| Files | 500 MB |
| Sessions, Agents, Environments | 32 MB |

---

## SDKs

### Official Client SDKs

| Language | Notes |
|----------|-------|
| Python | Sync and async clients, Pydantic models |
| TypeScript | Node.js, Deno, Bun, and browser support |
| C# | .NET Standard 2.0+, IChatClient integration — see below |
| Go | Context-based cancellation, functional options |
| Java | Builder pattern, CompletableFuture async |
| PHP | Value objects, builder pattern |
| Ruby | Sorbet types, streaming helpers |

### C# SDK

Package: `dotnet add package Anthropic` (NuGet)
Repo: [github.com/anthropics/anthropic-sdk-csharp](https://github.com/anthropics/anthropic-sdk-csharp)

**Status:** Beta (v10+). v10+ is the official Anthropic SDK; versions 3.x and below were a community build (`tryAGI`) — now at `tryAGI.Anthropic`.

**Key features:**
- `async`/`await` throughout; streaming via `IAsyncEnumerable`
- Auto-retry (2 attempts by default) with exponential backoff
- Default 10-minute timeout; both configurable globally or per-call via `WithOptions(...)`
- Typed exception hierarchy (`AnthropicRateLimitException`, `Anthropic5xxException`, etc.)
- `IChatClient` integration for Microsoft.Extensions.AI — plugs into the MCP C# SDK for tool-calling workflows
- Separate NuGet packages for Bedrock (`Anthropic.Bedrock`), Vertex AI (`Anthropic.Vertex`), Foundry (`Anthropic.Foundry`), and Claude Platform on AWS (`Anthropic.Aws`)

**Minimal example:**

```csharp
using Anthropic;
using Anthropic.Models.Messages;

AnthropicClient client = new(); // reads ANTHROPIC_API_KEY env var

var message = await client.Messages.Create(new()
{
    Model = Model.ClaudeOpus4_8,
    MaxTokens = 1024,
    Messages = [new() { Role = Role.User, Content = "Hello, Claude" }],
});
Console.WriteLine(message);
```

**Streaming:**

```csharp
await foreach (var chunk in client.Messages.CreateStreaming(parameters))
{
    Console.WriteLine(chunk);
}
```

---

## Access Paths (Cloud Platforms)

| Platform | Provider | Notes |
|----------|----------|-------|
| Direct API | Anthropic | Full feature access, latest models |
| Claude Platform on AWS | AWS (Anthropic-operated) | Supports Managed Agents |
| Amazon Bedrock | AWS (partner-operated) | Feature subset; 20 MB request limit |
| Vertex AI | Google Cloud (partner-operated) | Feature subset; 30 MB request limit |
| Microsoft Foundry | Azure (Anthropic-operated) | |

Managed Agents is only available on the direct API and Claude Platform on AWS.

---

## iOS / Apple

### No native Swift SDK (as of June 2026)

There is no general-purpose Swift/iOS SDK. Options for iOS today:

1. **REST via URLSession** — call `https://api.anthropic.com/v1/messages` directly with `URLSession`. Simple, no dependencies.
2. **OpenAI SDK compatibility** — Anthropic exposes an OpenAI-compatible endpoint; you can use an OpenAI Swift library and point it at Anthropic's base URL.
3. **Proxy through your backend** — your iOS app calls your own server, which calls the Anthropic API. Keeps the API key off the device (recommended for production).

### ClaudeForFoundationModels (coming — requires iOS 27 beta)

**Repo:** [github.com/anthropics/ClaudeForFoundationModels](https://github.com/anthropics/ClaudeForFoundationModels)
**Status:** Beta. Requires iOS 27 / macOS 27 / Xcode 27 (all in beta as of June 2026).
**License:** Apache 2.0

This is a Swift package that conforms Claude to Apple's `LanguageModel` protocol, letting you use Claude and Apple's on-device model interchangeably via the same `LanguageModelSession` API.

**Install (Package.swift):**

```swift
dependencies: [
  .package(url: "https://github.com/anthropics/ClaudeForFoundationModels.git", from: "0.1.0")
]
```

Or in Xcode: **File ▸ Add Package Dependencies…** → paste the URL above.

**Quick start:**

```swift
import FoundationModels
import ClaudeForFoundationModels

let model = ClaudeLanguageModel(
    name: .sonnet4_6,
    auth: .apiKey(ProcessInfo.processInfo.environment["ANTHROPIC_API_KEY"] ?? "")
)

let session = LanguageModelSession(model: model)
let response = try await session.respond(to: "Plan a 4-day trip to Buenos Aires.")
print(response.content)
```

**Streaming:**

```swift
let stream = session.streamResponse(to: "Summarize today's top science stories.")
for try await partial in stream {
    print(partial.content)  // cumulative snapshot, not a delta
}
```

**Structured output (`@Generable`):**

```swift
@Generable
struct Trip {
    @Guide(description: "Destination city") var destination: String
    @Guide(description: "Length in days") var days: Int
}

let response = try await session.respond(to: "Plan a trip to Tokyo.", generating: Trip.self)
print(response.content.destination)
```

**Server-side tools (web search, code execution):**

```swift
let model = ClaudeLanguageModel(
    name: .sonnet4_6,
    auth: auth,
    serverTools: [
        .webSearch(maxUses: 5),
        .codeExecution,
    ]
)
// Server tools run on Anthropic's infrastructure — nothing invoked on device
```

**Authentication in production (proxy pattern):**

```swift
// Never ship an API key in the binary — use a proxy instead
ClaudeLanguageModel(
    name: .sonnet4_6,
    auth: .proxied(headers: ["X-App-Token": "..."]),
    baseURL: URL(string: "https://api.yourapp.com/claude")!
)
// Your proxy receives standard Messages API requests, adds x-api-key, forwards to Anthropic
```

**Model selection:**

```swift
// Use a compiled-in constant (carries correct capabilities automatically)
ClaudeLanguageModel(name: .opus4_8, auth: auth)

// Pin effort level (takes precedence over framework's per-request reasoning hints)
ClaudeLanguageModel(name: .opus4_8, auth: auth, fixedEffort: .xhigh)
// Available levels: .low, .medium, .high, .xhigh, .max

// Custom/unreleased model — declare capabilities explicitly
let model = ClaudeModel(
    id: "claude-experimental-x",
    capabilities: .init(effortLevels: [.low, .high], structuredOutput: true)
)
```

**Error handling:**

```swift
do {
    let response = try await session.respond(to: prompt)
    print(response.content)
} catch ClaudeError.missingCredential {
    // Prompt for API key
} catch let error as LanguageModelError {
    // Rate limits, guardrails, context length, decoding — framework-shaped
} catch {
    // Transport errors
}
```

**When to use Claude vs. Apple's on-device model:**

| Use on-device (`SystemLanguageModel`) | Use Claude (`ClaudeLanguageModel`) |
|--------------------------------------|-------------------------------------|
| Fast, private, works offline | Larger context window needed |
| Lightweight tasks | Frontier reasoning needed |
| No API key required | Server-side tools (web search, code exec) |

**Not supported through the Foundation Models surface:**
- Prompt caching controls (applied automatically; TTL/breakpoints not configurable)
- Stop sequences
- Batch processing
- Files API
- Token counting
- Beta headers

---

## Rate Limits

Organized into usage tiers with per-organization spend limits and RPM/TPM caps. Tiers increase automatically with usage. Visible at [platform.claude.com/settings/limits](https://platform.claude.com/settings/limits).

---

## References

- [API Overview](https://platform.claude.com/docs/en/api/overview)
- [SDK Overview](https://platform.claude.com/docs/en/cli-sdks-libraries/overview)
- [C# SDK Docs](https://platform.claude.com/docs/en/cli-sdks-libraries/sdks/csharp)
- [Apple Foundation Models](https://platform.claude.com/docs/en/cli-sdks-libraries/libraries/apple-foundation-models)
- [ClaudeForFoundationModels on GitHub](https://github.com/anthropics/ClaudeForFoundationModels)
- [Rate Limits](https://platform.claude.com/docs/en/api/rate-limits)
