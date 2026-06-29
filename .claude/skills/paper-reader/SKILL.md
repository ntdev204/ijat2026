---
name: paper-reader
description: >
  Read and analyze academic papers 5x faster. Use this skill ANY TIME the user mentions reading a paper, summarizing research, understanding a method, literature review, or asks anything about a specific paper — even "read this", "what does this paper do", "scan this paper", "should I read this?", or "explain this research". Supports PDF upload, arXiv link, or pasted text/abstract. Always trigger for academic papers, research articles, or preprints — even when only the abstract is available.
---

# Paper Reader Skill

Goal: help researchers fully understand any paper 5x faster — from filter decisions to deep method analysis.

---

## Language

- Analysis, commentary, explanations → Vietnamese
- Technical terms, model names, metrics, datasets, author names, citations → keep in English
- Q&A mode → match the user's language

---

## Input Handling

| Input type | Action |
|---|---|
| PDF upload | Read directly from file |
| arXiv link | `web_fetch` → `https://ar5iv.labs.arxiv.org/abs/<id>` for full HTML text |
| Pasted text/abstract | Analyze from provided text; note missing info clearly |

**arXiv fetch priority:**
1. Full HTML: `https://ar5iv.labs.arxiv.org/abs/XXXX.XXXXX` ← preferred
2. Abstract: `https://arxiv.org/abs/XXXX.XXXXX`
3. PDF: `https://arxiv.org/pdf/XXXX.XXXXX`

---

## Workflow

```
User sends paper (no specific question)
    │
    ▼
⚡ 60-Second Scan  ← ALWAYS run first
    │
    ├─ User: "stop / skip / enough" → stop here
    └─ User: "continue / full analysis / yes" → run Full Analysis

User asks a specific question about the paper
    └─ Q&A Mode (answer directly)

User says "quick summary" / only abstract available
    └─ Quick Mode (Scan + short Gaps)
```

**Do not auto-continue to Full Analysis.** Always stop after the Scan and wait for the user to explicitly request more.

---

## Mode 1: 60-Second Scan (ALWAYS run first)

Goal: help a researcher decide whether to read or skip a paper in 60 seconds.

```
⚡ 60-SECOND SCAN
─────────────────────────────────────
Problem      : [1 sentence — the problem being solved]
Core Idea    : [1 sentence — the main solution]
Contribution : • [key contribution 1]
               • [key contribution 2]
Main Result  : [top metric — exact from paper]
Why It Matters: [1 sentence — why this matters to the field]
─────────────────────────────────────
```

### 📖 Should You Read This Paper?

```
📖 SHOULD YOU READ THIS PAPER?
─────────────────────────────────────
✅ Read if:
  - [reader profile 1 who benefits]
  - [reader profile 2 who benefits]

⏭️ Skip if:
  - [case where it's not worth reading 1]
  - [case where it's not worth reading 2]

Estimated relevance: High / Medium / Low
─────────────────────────────────────
```

After the Scan → ask: *"Want a deeper analysis?"* Then wait for the user's response.

---

## Mode 2: Full Analysis

Run sections in order after user confirms.

---

### Section 1 — Structured Info Extraction

*(For literature review, knowledge base, survey)*

```
📊 KEY PAPER INFORMATION
─────────────────────────────────────
Field    : [e.g., Robotics / Reinforcement Learning]
Method   : [main method / approach name]
Datasets : [dataset names, or "Not specified"]
Baselines: [models compared against]
Metrics  : [key evaluation metrics]
Code     : [Released / Not released / link if available]
Venue    : [e.g., ICRA 2024 / arXiv preprint]
─────────────────────────────────────
```

---

### Section 2 — Structured Breakdown

**Problem & Motivation**
- What problem is being solved?
- Why are prior methods insufficient?

**Method** *(keep all technical terms in English)*
- Main architecture / pipeline
- Key innovations — what is genuinely different from prior work
- Important components (loss function, training strategy, etc.)

**Experiments & Results**
- Datasets and baselines used
- Key numerical results (quote exactly from paper — do not guess)

---

### Section 3 — Method Pipeline

Visualize the pipeline as a flow to quickly understand the method:

```
⚙️ METHOD PIPELINE

[Input]
  → [Module 1: e.g., Perception / Feature Extraction]
  → [Module 2: e.g., Core Algorithm / Policy Network]
  → [Module 3: e.g., Decision / Control]
  → [Output]

Key design choice: [1 sentence explaining the most important design decision]
```

For branching or parallel pipelines, use indented bullets:
```
[Input]
  → Branch A: [...]
  → Branch B: [...]
  → Fusion: [...]
  → [Output]
```

---

### Section 4 — Limitations

**Author-stated limitations:**
- [what the authors themselves acknowledge in the paper]

**Additional limitations (reviewer perspective):**
- [what the authors did not mention — assumptions, scalability, missing comparisons]
- Assumptions that may not hold in practice
- Scalability / generalization concerns
- Missing comparisons or evaluation gaps

---

### Section 5 — Claim vs Evidence Check

*(Critical reading — detect overclaims common in ML papers)*

| Claim | Evidence | Issue |
|---|---|---|
| [Author's claim] | [Supporting Table/Figure] | [Problem if any, or "OK"] |

Common patterns to check:
- "Outperforms all SOTA" → only tested on 1 dataset?
- "Real-time capable" → benchmarked on A100, not edge hardware?
- "Generalizes well" → only tested in-distribution?

If no significant overclaims found → write: *"No major overclaims detected."*

---

### Section 6 — Research Gaps

**Experimental Gaps**
- Missing ablation studies
- Missing robustness / stress tests
- Unexplored hyperparameter sensitivity

**Method Gaps**
- Theoretical limitations not addressed
- Missing components (e.g., uncertainty modeling, real-time constraints)
- Unexplored combinations with other techniques

**Application Gaps**
- Scenarios not tested (e.g., real robots vs. simulation only)
- Domain transfer not explored
- Edge cases / deployment scenarios

> 💡 **Research Ideas** *(Claude's opinion — not from the paper)*
> 1. [specific, actionable idea]
> 2. [specific, actionable idea]
> 3. [specific idea — prioritize ROS2/Robotics relevance if applicable]

---

### Section 7 — Practical Impact

```
🌍 PRACTICAL IMPACT
─────────────────────────────────────
Applications:
  - [use case 1]
  - [use case 2]

Real-world constraints:
  - [hardware / compute requirements]
  - [data requirements]
  - [deployment complexity]

Maturity: [Research prototype / Near deployment / Production-ready]
─────────────────────────────────────
```

---

### Section 8 — Citation Context & Related Work

**Citation Context**

```
📚 CITATION CONTEXT
─────────────────────────────────────
Builds upon:
  - [Paper A (year)] — [brief reason]
  - [Paper B (year)] — [brief reason]

Research influence: [Foundational / Active direction / Niche]
─────────────────────────────────────
```

**Closest Prior Work:**

| Paper | Method | Key Difference vs. This Work | Year |
|---|---|---|---|
| ... | ... | ... | ... |

**Recent Related Work (≤2 years):**

| Paper | Method | Key Difference vs. This Work | Year |
|---|---|---|---|
| ... | ... | ... | ... |

> If the paper is recent or tables need updating → `web_search` on arXiv / Semantic Scholar first.

---

## Mode 3: Q&A Mode

When the user asks a specific question about the paper:

```
❓ [Question]

📄 From the paper: [answer grounded in paper content]
💭 Note: [additional context or analysis if needed]
```

If the paper does not address the question → say so clearly and suggest where to look.

---

## Mode 4: Quick Mode

When the user says "quick summary" or only an abstract is available:
1. Run **60-Second Scan** + **Should You Read This?**
2. Add **Research Gaps** (3–5 bullets combined, no categorization)
3. Skip all remaining sections

---

## Quality Checklist (self-check before output)

- [ ] 60-Second Scan always runs first; ask user before Full Analysis; wait for explicit request
- [ ] Structured Info complete: Field / Method / Datasets / Metrics / Code / Venue
- [ ] Method Pipeline clearly visualizes the flow
- [ ] Limitations split: author-stated vs. additional
- [ ] Claim vs Evidence: at least 2–3 claims checked
- [ ] Research Gaps cover all 3 categories
- [ ] Related Work split: Closest Prior vs. Recent (≤2 years)
- [ ] All technical terms kept in English
- [ ] 💡 Research Ideas clearly marked as Claude's opinion
- [ ] No fabricated numbers — write "not reported in paper" if uncertain
