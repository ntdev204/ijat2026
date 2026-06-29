---
name: paper-writer
description: >
  Full-cycle academic paper writing assistant covering any domain, with deep expertise in AI/ML and IEEE formats.
  Use this skill whenever the user wants to write, draft, outline, structure, or improve an academic paper — even if they only mention a topic idea, research question, or rough notes.
  Triggers include: "write a paper about", "help me draft a paper", "IEEE format", "conference paper", "journal article", "LaTeX paper", "research paper outline", "abstract for my paper", "related work section", "write introduction", "paper on [any topic]", "publication draft", or any mention of submitting to a venue (conference, journal, transactions).
  Produces both LaTeX (.tex) and Markdown drafts, with BibTeX citations and IEEE-style references. Handles all IEEE formats (Conference, Journal, Transactions, Workshop) and any academic domain.
  Always use this skill when the user wants any form of academic writing help — even partial (just an abstract, just an outline, just a related work section).
---

# Paper Writer Skill

A full-cycle academic paper writing assistant. Handles everything from a raw idea to a submission-ready LaTeX + Markdown draft with proper IEEE formatting and citations.

---

## Workflow Overview

1. **Intake** — Understand what the user has and what they need
2. **Literature Mapping** — Search, cluster, surface gaps ← do before framing
3. **Research Questions** — Derive RQ1/RQ2/RQ3 from the gap ← anchors the whole paper
4. **Research Framing** — Contribution type, novelty, hypothesis, elevator pitch ← CRITICAL
5. **Figure Planning** — Plan all figures before writing a single word
6. **Outline** — Build a structured paper outline for approval
7. **Baseline Selection** — Choose and justify comparison baselines
8. **Experiment Design** — Define metrics, datasets, ablations
9. **Draft** — Write section by section (or full paper at once)
10. **Citations** — Search for real papers (last 5 years prioritized), generate BibTeX
11. **Format** — Produce IEEE-compliant LaTeX + Markdown output
12. **Failure Mode Analysis** — Identify and document limitations honestly
13. **Reviewer Critique** — Self-review as a hostile reviewer before finalizing
14. **Iterate** — Refine based on feedback

---

## Step 1: Intake

The user may provide any of the following — adapt accordingly:

| Input | Action |
|---|---|
| Just a topic/idea | Ask for domain, target venue, and key contribution |
| Research question + contributions | Go straight to outline |
| Existing notes or bullet points | Extract structure, confirm with user |
| Partial draft | Identify gaps, complete missing sections |

**Always ask (if not provided):**
- Target venue type: Conference / Journal / Transactions / Workshop?
- Domain: AI/ML, Robotics, EE, CS, other?
- Approximate page limit (IEEE: typically 4–8 pages for conference, 8–12 for journal)
- Any existing related work or papers to cite?

---

## Step 2: Literature Mapping

**Goal:** Before claiming a gap exists, prove it exists. Use web search to build a map of the field, then systematically find what is missing. Do this BEFORE Research Framing.

### Phase A — Search & Cluster

Search for papers across 3 layers:

```
Layer 1 — Core topic:    "[your exact problem]" + 2022..2025
Layer 2 — Adjacent:      "[related technique]" + "[your domain]" + 2022..2025
Layer 3 — Foundational:  "[seminal method]" — defines baseline everyone builds on
```

For each layer, collect 5–10 papers. Then cluster them by **what they solve**:

```
LITERATURE MAP
==============
Cluster A: [Sub-theme 1, e.g. "real-time detection"]
  - [Paper1, Year] — solves X, fails at Y
  - [Paper2, Year] — solves X+Z, fails at W

Cluster B: [Sub-theme 2, e.g. "transformer-based approaches"]
  - [Paper3, Year] — solves ...

UNCOVERED COMBINATIONS:
  - No paper addresses [A + B] simultaneously
  - All methods assume [condition], but real-world violates it
  - [Metric] never reported despite being critical for deployment
```

### Phase B — Gap Detection

After clustering, identify gaps using these 6 patterns:

| Gap Type | Signal | Example |
|---|---|---|
| **Missing combination** | "X and Y separately, never together" | Accuracy + real-time never combined |
| **Assumption violation** | "All assume Z, but Z is false in practice" | Assume clean labels, real data is noisy |
| **Missing metric** | "No one measures W, but W is critical" | FLOPs never reported in medical imaging |
| **Domain transfer** | "Works in domain A, untested in domain B" | Works on ImageNet, fails on satellite |
| **Scale gap** | "Works on toy setting, not at scale" | Tested on 100 samples, need 1M |
| **Efficiency gap** | "Accurate but too slow/large to deploy" | 97% accuracy but 10GB model |

### Phase C — Gap Statement

Produce one crisp gap statement before moving to framing:

```
GAP STATEMENT
=============
Existing work: [What has been solved]
Missing:       [What has NOT been solved — one gap type from above]
Evidence:      [2-3 citations that confirm this gap]
Opportunity:   [Why solving this gap matters]
```

> ⚠️ Rule: If you cannot point to at least 2 papers confirming the gap by their absence or limitation, search more. Do not proceed with an unverified gap.

---

## Step 3: Research Questions

**Goal:** Convert the Gap Statement into 2–4 focused, answerable research questions. RQs anchor every section — the method answers them, experiments measure them, conclusion reflects on them.

### Rules for Good RQs

| Rule | Bad RQ | Good RQ |
|---|---|---|
| Specific, not vague | "Can AI improve robotics?" | "Can a transformer controller reduce tracking error under unseen disturbances?" |
| Answerable by experiments | "Is our method better?" | "Does X outperform Y on metric Z in dataset D?" |
| Scoped, not open-ended | "What are the limits of deep learning?" | "Does performance degrade below 64×64 resolution?" |
| One question per RQ | "Does X work and why?" | Split: RQ1 = does it work, RQ2 = why |

### RQ Generation Template

Derive RQs directly from the Gap Statement. Each RQ maps to one cluster in the Literature Map.

```
RESEARCH QUESTIONS
==================
Gap recap: [One sentence from Gap Statement]

RQ1 (Primary — proves main contribution):
  "Can/Does/How well does [method] improve [metric] under [condition]?"
  Answered by: [main results table / section]

RQ2 (Generalization — tests robustness):
  "Does [method] maintain performance when [distribution/domain/scale changes]?"
  Answered by: [cross-dataset / out-of-distribution experiment]

RQ3 (Component — justifies design choices):
  "Does [component A] contribute more than [component B] to [metric]?"
  Answered by: [ablation study]

RQ4 (optional — practical / deployment):
  "Can [method] meet [real-time / memory / edge] requirements under [constraint]?"
  Answered by: [efficiency / latency experiment]
```

### Connecting RQs to the Paper

```
RQ -> Paper Mapping
====================
RQ1 -> Method Section + Main Results Table
RQ2 -> Generalization / Cross-dataset Experiment
RQ3 -> Ablation Study
RQ4 -> Computational Cost Table
```

> ⚠️ Rule: Every RQ must be answered with a number in experiments. "We believe RQ2 is satisfied" is not an answer — redesign the experiment or drop the RQ.

> ⚠️ Rule: If the user has no RQs yet, generate candidates from the Gap Statement and ask for confirmation before proceeding.

---

## Step 4: Research Framing ⚠️ NEVER SKIP

**This is the most important step.** A paper that sounds good but lacks novelty will be desk-rejected. Before writing a single sentence, force the model to answer all five questions below. If the user cannot answer them, help them think through it — do NOT proceed to writing until this is solid.

### The 5 Framing Questions

| # | Question | Why it matters |
|---|---|---|
| 1 | **What is the exact problem?** | Vague problems → vague papers |
| 2 | **What is the gap in existing literature?** | Defines novelty |
| 3 | **Why do existing methods fail at this?** | Justifies the work |
| 4 | **What is your hypothesis / proposed solution?** | Core contribution |
| 5 | **How will you evaluate success?** | Makes the paper falsifiable |

### Output a Research Framing Block

Always produce this block explicitly and show it to the user for confirmation before proceeding:

```
RESEARCH FRAMING
================
Problem:      [1-2 sentences, specific and measurable]
Gap:          [What existing work does NOT solve, with citations]
Why gap exists: [Technical/practical reason prior work fails]
Hypothesis:   [If we do X, then Y will improve because Z]
Contribution Type: [algorithm / framework / dataset / theory / analysis / system]
Contribution: 1. [Novel method/framework/dataset/analysis]
              2. [...]
              3. [...]
Evaluation:   [Metrics + datasets + baselines that will prove/disprove hypothesis]

Elevator Pitch: "This paper proposes [METHOD] which solves [PROBLEM] by [MECHANISM],
                 achieving [KEY RESULT] on [BENCHMARK]."
```

### Elevator Pitch Rule

The elevator pitch is a single sentence that must survive the "cocktail party test" — if you cannot explain your paper to a peer in one sentence, the contribution is not yet clear enough to write.

**Template:**
```
This paper proposes [X: name your method]
which solves [Y: state the problem / gap]
by [Z: describe the key mechanism — NOT just "using deep learning"]
achieving [W: one quantitative result or capability gain]
on [V: dataset / benchmark / domain].
```

**Examples:**
- ✅ "This paper proposes FuzzyTrack, which solves unstable UAV trajectory tracking under wind disturbances by combining adaptive fuzzy rules with a model predictive controller, reducing tracking error by 34% on the AirSim benchmark."
- ✅ "This paper proposes GraphMed, which solves cross-modality medical image segmentation by learning shared anatomical graph representations, achieving state-of-the-art Dice score on three public datasets."
- ❌ "This paper proposes a new deep learning framework that improves performance." — too vague, no mechanism, no result.

> ⚠️ Rule: The elevator pitch must be written BEFORE drafting the abstract. The abstract expands it; it does not replace the need to have one.

### Contribution Type Classification

Every paper MUST declare exactly one primary contribution type. This shapes how the paper is written and evaluated.

| Type | Definition | Typical evidence in paper |
|---|---|---|
| **algorithm** | New procedure / method / model | Pseudocode + accuracy improvement |
| **framework** | Reusable system design / pipeline | Architecture diagram + case studies |
| **dataset** | New labeled collection of data | Statistics + baseline benchmarks on it |
| **theory** | Proof / bound / formal analysis | Theorems + proofs + empirical validation |
| **analysis** | Empirical study of existing methods | Systematic experiments + findings |
| **system** | End-to-end deployed system | Latency + throughput + real-world deployment |

**Rule:** Once declared, the contribution type determines what reviewers will demand:
- `algorithm` → they will ask: is the accuracy gain significant? is there an ablation?
- `framework` → they will ask: is it general? does it work on multiple tasks?
- `dataset` → they will ask: how was it collected? what biases exist? what baselines?
- `theory` → they will ask: are assumptions realistic? does empirical result match theory?
- `analysis` → they will ask: is the finding surprising? is the methodology rigorous?
- `system` → they will ask: does it run in real-time? what are the deployment constraints?

> ⚠️ Red flag: A paper trying to claim ALL types simultaneously usually proves none well. Pick one primary type, others can be secondary.

### Novelty Validation

After framing, run this check. If any answer is "no", revise before proceeding:

- [ ] Is the contribution **new**? (not just an application of existing method)
- [ ] Is the contribution **verifiable**? (can be tested in experiments)
- [ ] Does each contribution **appear in the method AND experiments**?
- [ ] Would a reviewer say "I haven't seen this exact thing before"?

> ⚠️ **Red flag:** If the framing sounds like *"We apply [existing method] to [new dataset]"* — that is likely insufficient novelty for IEEE Transactions or top conferences. Push the user to identify a deeper technical contribution.

---

## Step 5: Choose IEEE Format

See `references/ieee-formats.md` for detailed format specs.

| Format | Template Class | Typical Length | Notes |
|---|---|---|---|
| IEEE Conference | `IEEEtran` (conference mode) | 4–8 pages | Double column |
| IEEE Journal | `IEEEtran` (journal mode) | 8–12 pages | Double column |
| IEEE Transactions | `IEEEtran` (transactions mode) | 10–14 pages | Strict style guide |
| IEEE Workshop | `IEEEtran` (conference mode) | 2–4 pages | Condensed structure |

---

## Step 6: Figure Planning

**Plan all figures BEFORE writing.** Figures are the skeleton of the paper — readers look at them first. If the figures tell a coherent story, the writing fills in the gaps. If figures are an afterthought, the paper feels disjointed.

### Figure Planning Rules

1. Every major claim needs a figure or table — never text-only claims
2. The architecture/system figure is mandatory for method papers
3. Result figures must visually show the answer to at least one RQ
4. Plan 4–7 figures for a conference paper, 6–10 for a journal

### Figure Plan Template

Produce this block before writing a single section:

```
FIGURE PLAN
===========
Fig 1: [MOTIVATION / PROBLEM]
  Type:    Diagram / illustration
  Shows:   Why the problem is hard / what gap looks like visually
  Caption: "Existing methods fail because X. Our approach Y addresses this by Z."
  Section: Introduction

Fig 2: [ARCHITECTURE / SYSTEM OVERVIEW]  ← mandatory for method papers
  Type:    Block diagram / pipeline
  Shows:   Full method — inputs, components, outputs, data flow
  Caption: "Overview of [method name]. Given X, the system produces Y via Z."
  Section: Method

Fig 3: [KEY TECHNICAL DETAIL]
  Type:    Diagram / equation visualization / attention map
  Shows:   The novel component that makes the method work
  Caption: "Detail of [component]. Unlike prior work, our design achieves..."
  Section: Method

Fig 4: [MAIN RESULTS]
  Type:    Bar chart / line plot / table (if too many numbers for chart)
  Shows:   Our method vs all baselines on primary metric
  Caption: "[Method] outperforms all baselines on [metric]. Best result in bold."
  Section: Experiments

Fig 5: [ABLATION]
  Type:    Bar chart / table
  Shows:   Each component's contribution
  Caption: "Ablation study. Removing component X drops performance by Y%."
  Section: Experiments

Fig 6: [QUALITATIVE / VISUAL RESULT]
  Type:    Image grid / trajectory plot / prediction overlay
  Shows:   What success looks like visually + one failure case
  Caption: "Qualitative results on [dataset]. Last row shows a failure case where..."
  Section: Experiments

Fig 7 (optional): [EFFICIENCY / COST]
  Type:    Scatter plot (accuracy vs latency) or table
  Shows:   Our method achieves better trade-off than baselines
  Caption: "Accuracy-latency trade-off. Our method achieves X accuracy at Y ms."
  Section: Experiments / Discussion
```

### Figure Quality Rules

- Every figure must have a **self-contained caption** — reader should understand it without reading the body
- Failure cases are not optional — include at least one in qualitative results
- Use consistent colors across all figures (define a palette: primary color for "ours", gray for baselines)
- Vector format (PDF/SVG) for diagrams; PNG at ≥300 DPI for photos/screenshots
- Never use a figure without referencing it in the text before it appears

> ⚠️ Rule: If you cannot plan at least 4 figures with clear captions, the paper does not yet have enough content. Go back to the Research Framing and expand the contributions.

---

## Step 7: Build the Outline

Always present an outline before drafting. Standard IEEE structure:

```
Title
Authors & Affiliations
Abstract (150–250 words)
Index Terms / Keywords

I. Introduction
   - Motivation & problem statement
   - Key contributions (bulleted)
   - Paper organization

II. Related Work
   - Grouped by sub-theme
   - Position vs. prior art

III. [Method / System / Approach]
   - Architecture / framework
   - Technical details
   - Algorithms (if any)

IV. Experiments / Evaluation
   - Setup & datasets
   - Metrics
   - Results & analysis

V. Discussion (optional, more common in journals)

VI. Conclusion
   - Summary
   - Future work

References (IEEE style)

Appendix (optional)
```

Adapt section names to the domain (e.g., "Methodology", "System Design", "Proposed Framework").

---

## Step 8: Baseline Selection Engine

Reviewers will ask: *"Why didn't you compare to X?"* — select baselines proactively.

### Baseline Selection Rules

1. **Always include the current SOTA** for the task (search for it if unknown)
2. **Include the most-cited method** in the sub-field (even if older)
3. **Include one simple/naive baseline** (e.g., linear model, threshold-based) to show non-trivial gains
4. **Include ablations of your own method** (remove each component one by one)

### Baseline Justification Template

For each baseline, document:
```
Baseline: [Name, Year, Venue]
Why included: [SOTA / most-cited / ablation / naive]
Why our method should outperform: [specific technical reason]
Expected delta: [qualitative: large / marginal / task-dependent]
```

### Baseline Search Strategy
- Search: `"[task name] state of the art 2024"` + `"[task] benchmark leaderboard"`
- Check: Papers With Code (paperswithcode.com) for leaderboards
- For AI/ML: NeurIPS/ICML/ICLR/CVPR last 2 years
- For Robotics: ICRA/IROS last 2 years

> ⚠️ **Rule:** Never select baselines only from your own lab or only older than 3 years without justification.

---

## Step 9: Experiment Design Generator

Strong experiments answer the framing hypothesis. Weak experiments just show numbers.

### Experiment Design Template

```
PRIMARY EXPERIMENT
==================
Goal:       Prove main contribution (links to Hypothesis in Step 2)
Dataset:    [Name, size, split: train/val/test]
Metric:     [Primary metric + secondary metrics]
Baselines:  [From Step 5]
Expected result: [Our method > all baselines on primary metric]

ABLATION STUDY
==============
Goal:       Prove each component contributes
Variants:   - Full model (ours)
            - Remove component A → shows A matters
            - Remove component B → shows B matters
            - Replace C with simple alternative → shows C design is optimal

ANALYSIS / QUALITATIVE
=======================
Goal:       Understand when/why method works or fails
Include:    - Failure cases (shows honesty, reviewers appreciate this)
            - Visualization (attention maps, prediction examples, etc.)
            - Computational cost table (params, FLOPs, latency)
```

### Metric Selection Rules
- Use metrics standard in your sub-field (don't invent new ones without justification)
- Report mean ± std over multiple runs (minimum 3 seeds for DL)
- For real-time systems: always report inference latency (ms) and hardware spec
- For generation tasks: use multiple metrics (e.g., BLEU + ROUGE + human eval)

---

## Step 10: Draft Sections

Write each section with these principles:

### Abstract
- 150–250 words for IEEE
- Structure: problem → gap → method → results → impact
- No citations, no figures references
- Must standalone — reader should understand the paper from abstract alone

### Introduction
- Open with broad motivation (1–2 paragraphs)
- Narrow to specific problem
- State contributions **explicitly** as a numbered list — this phrasing is mandatory:
  ```
  The main contributions of this paper are:
  1. We propose ...
  2. We introduce ...
  3. We demonstrate ...
  ```
- **Contribution validation rule:** Every contribution listed here MUST appear in either the Method section or the Experiments section. If a contribution cannot be pointed to in the paper body, remove it.
- End with paper organization paragraph

### Related Work
- Group by theme, not chronologically
- Each paragraph covers one sub-theme
- End each paragraph positioning your work vs. prior art
- Minimum 8–12 citations for conference, 15–25 for journal

### Method/System Section
- Start with an overview diagram description (suggest figure)
- Break into subsections for each component
- Use equations for any mathematical formulation (IEEE LaTeX `equation` environment)
- Algorithm blocks where appropriate (`algorithm` + `algorithmic` packages)

### Experiments
- Restate setup clearly (dataset, hardware, baselines)
- Present results in tables (IEEE `table` environment)
- Analyze results — don't just describe numbers
- Ablation studies if applicable

### Conclusion
- 1–2 paragraphs max
- Summarize key findings
- State concrete future work directions

---

## Step 11: Citations

### Recency Rule ⚠️
> **Default: prefer papers from the last 5 years (2020–2025).**
> Foundational works are the only exception (e.g., Transformer 2017, ResNet 2016, ROS 2009).
> If citing a paper older than 5 years, be ready to justify: *"This is a foundational/seminal work that all subsequent methods build on."*
> Reviewers will ask: *"Why do you cite outdated work?"* — pre-empt this.

### Citation Age Audit
Before finalizing references, run this check:
- Count papers per year range: 2020–2025, 2015–2019, pre-2015
- Target ratio: **≥70% from last 5 years**
- Flag any pre-2015 citation and confirm it is truly foundational

### Search Strategy
Use web search to find real, citable papers:
- Search Google Scholar, arXiv, IEEE Xplore, Semantic Scholar, Papers With Code
- Prefer: peer-reviewed > arXiv preprints > technical reports
- **Always search for the year**: include "2023" or "2024" in queries to get recent work
- For AI/ML topics: arXiv + NeurIPS/ICML/ICLR/CVPR proceedings (last 3 years)
- For Robotics/ROS2: ICRA, IROS, RA-L (last 3 years)

### BibTeX Generation
Generate proper BibTeX for every cited paper:

```bibtex
@inproceedings{lastname2024keyword,
  author    = {Last, First and Last2, First2},
  title     = {Full Paper Title Here},
  booktitle = {Proceedings of the IEEE Conference on ...},
  year      = {2024},
  pages     = {1234--1241},
  doi       = {10.1109/...}
}

@article{lastname2023keyword,
  author  = {Last, First},
  title   = {Full Article Title},
  journal = {IEEE Transactions on ...},
  year    = {2023},
  volume  = {XX},
  number  = {Y},
  pages   = {1--15},
  doi     = {10.1109/...}
}
```

### IEEE Citation Style
- Numbered citations: `[1]`, `[2]`, `[1]–[3]`
- In LaTeX: `\cite{key}`, `\citep` not used in IEEE
- References section: use `IEEEtran` bibliography style

---

## Step 12: LaTeX Output

### Document Structure

```latex
\documentclass[conference]{IEEEtran}  % or [journal] / [transactions]

% Core packages
\usepackage{cite}
\usepackage{amsmath,amssymb,amsfonts}
\usepackage{algorithmic}
\usepackage{algorithm}
\usepackage{graphicx}
\usepackage{textcomp}
\usepackage{xcolor}
\usepackage{hyperref}
\usepackage{booktabs}

\begin{document}

\title{Paper Title}

\author{
  \IEEEauthorblockN{Author Name}
  \IEEEauthorblockA{Department\\Institution\\City, Country\\email@domain.com}
  \and
  \IEEEauthorblockN{Author Name 2}
  \IEEEauthorblockA{...}
}

\maketitle

\begin{abstract}
...
\end{abstract}

\begin{IEEEkeywords}
keyword1, keyword2, keyword3
\end{IEEEkeywords}

\section{Introduction}
...

\bibliographystyle{IEEEtran}
\bibliography{references}

\end{document}
```

Always output:
1. `main.tex` — full LaTeX source
2. `references.bib` — BibTeX file
3. `paper_draft.md` — Markdown version for easy reading/editing

---

## Step 13: Markdown Output

Mirror the LaTeX structure in Markdown:
- Use `##` for sections, `###` for subsections
- Tables in GFM format
- Equations in `$$...$$` blocks
- Citations as `[AuthorYear]` with a References section at the bottom
- Note at top: `> 📄 LaTeX version available as main.tex`

---

## Step 14: Failure Mode Analysis

**Strong papers are honest papers.** Reviewers trust authors who acknowledge limitations. Hiding failure modes invites rejection; disclosing them with analysis earns respect.

### Mandatory Failure Mode Block

Before finalizing the paper, produce this block and include a condensed version in the Discussion or Conclusion section:

```
FAILURE MODE ANALYSIS
=====================
Mode 1: [Describe a specific input/condition where method degrades]
  Cause:    [Technical reason — e.g., distribution shift, edge case, assumption violated]
  Evidence: [Quantitative drop or qualitative example]
  Severity: [critical / moderate / minor]
  Mitigation: [What can be done — future work or existing workaround]

Mode 2: [...]
  Cause:    [...]
  Evidence: [...]
  Severity: [...]
  Mitigation: [...]

Mode 3: [Computational / resource failure mode]
  Cause:    [e.g., quadratic memory scaling, no real-time on edge devices]
  Evidence: [Latency table, memory profile]
  Severity: [...]
  Mitigation: [...]
```

### Failure Mode Discovery Checklist

Ask these questions to surface failure modes systematically:

**Data distribution failures**
- [ ] What happens when input is out-of-distribution?
- [ ] What happens with noisy / corrupted / missing input?
- [ ] Does performance degrade on minority classes or rare cases?

**Scale failures**
- [ ] Does the method slow down non-linearly with input size?
- [ ] Does memory usage become prohibitive at scale?
- [ ] Does accuracy drop on longer sequences / higher resolution / more classes?

**Assumption violations**
- [ ] What assumptions does the method make? (e.g., i.i.d., balanced classes, known domain)
- [ ] What happens when each assumption is violated?

**Deployment failures**
- [ ] Does it work on hardware weaker than the test environment?
- [ ] Is it sensitive to hyperparameters that require tuning per dataset?
- [ ] Does it require data that is hard to collect in practice?

### Writing Limitations in the Paper

In the Conclusion or a dedicated Limitations section:
- Be specific: *"Our method degrades by 12% F1 when..."* not *"may not generalize"*
- Pair each limitation with a mitigation or future direction
- Never bury limitations in a single vague sentence at the end

> ⚠️ Rule: Minimum 2 failure modes must be documented. "No limitations" is never acceptable — it signals the authors did not test carefully or are not being honest.

---

## Step 15: Reviewer-Style Critique

Before presenting the final draft to the user, simulate a **hostile but fair IEEE reviewer**. This catches fatal flaws before submission.

### Reviewer Persona
Pretend you are a senior IEEE reviewer who:
- Has read 200+ papers in this domain
- Is skeptical of overclaimed contributions
- Looks for missing baselines and weak experiments
- Checks citation recency
- Asks "so what?" after every claimed contribution

### Critique Template

Produce this block after drafting:

```
REVIEWER CRITIQUE (Pre-submission check)
=========================================
[NOVELTY]
  ✅/⚠️/❌  Is the contribution clearly novel vs. prior work?
  Note: [specific concern or confirmation]

[EXPERIMENTS]
  ✅/⚠️/❌  Are baselines sufficient and up-to-date?
  ✅/⚠️/❌  Do results actually prove the hypothesis?
  ✅/⚠️/❌  Is there an ablation study?
  Note: [missing comparisons, weak metrics, etc.]

[CITATIONS]
  ✅/⚠️/❌  Are ≥70% of citations from last 5 years?
  ✅/⚠️/❌  Is SOTA cited?
  Note: [outdated or missing key works]

[WRITING]
  ✅/⚠️/❌  Are contributions explicit and numbered?
  ✅/⚠️/❌  Does each contribution appear in method/experiments?
  ✅/⚠️/❌  Is the abstract self-contained?
  Note: [clarity, overclaiming, unsupported statements]

[FAILURE MODES]
  ✅/⚠️/❌  Are at least 2 failure modes documented?
  ✅/⚠️/❌  Are limitations specific (not vague)?
  ✅/⚠️/❌  Does the paper suggest mitigations?
  Note: [undisclosed failure modes the reviewer suspects]

[VERDICT]
  Likely outcome: Accept / Minor revision / Major revision / Reject
  Top 3 things to fix before submission:
  1. ...
  2. ...
  3. ...
```

> ⚠️ If verdict is **Reject**, do NOT present the paper to the user as final. Go back and fix the top issues first.

---

## Quality Checklist

Before presenting output, verify:

**Research Quality**
- [ ] Literature Map produced (clusters + Gap Statement with ≥2 confirming citations)
- [ ] Research Questions (RQ1–RQ3+) derived from Gap Statement
- [ ] Every RQ answered with a number in experiments
- [ ] Elevator Pitch written: "This paper proposes X which solves Y by Z achieving W on V"
- [ ] Research Framing block completed and confirmed by user
- [ ] Contribution Type declared: algorithm / framework / dataset / theory / analysis / system
- [ ] Novelty validated — contribution is new, not just an application
- [ ] Each contribution in Introduction appears in Method or Experiments
- [ ] Contributions are numbered ("The main contributions of this paper are: 1. 2. 3.")
- [ ] Figure Plan completed (≥4 figures, each with caption draft)
- [ ] Architecture/system overview figure planned (mandatory for method papers)
- [ ] At least one failure case included in qualitative results
- [ ] Baselines include SOTA + most-cited + naive baseline + ablations
- [ ] Experiments directly test the stated hypothesis
- [ ] Failure Mode Analysis completed (minimum 2 modes documented)
- [ ] Limitations written specifically in paper (not vague)

**Citations**
- [ ] ≥70% of citations are from last 5 years (2020–2025)
- [ ] All pre-2015 citations are foundational and justified
- [ ] Citation age audit completed
- [ ] All BibTeX entries have DOI or URL

**Writing**
- [ ] Abstract is self-contained and within word limit (150–250 words)
- [ ] All figures/tables referenced in text before appearance
- [ ] Every claim backed by a citation
- [ ] Related work positions paper vs. prior art
- [ ] Conclusion doesn't introduce new content

**Format**
- [ ] LaTeX compiles without errors
- [ ] IEEE two-column format respected
- [ ] Page limit respected (~500 words/column, 2 columns/page)

**Reviewer Critique**
- [ ] Reviewer critique block produced
- [ ] Verdict is not "Reject" before presenting to user

---

## Domain-Specific Notes

### AI / Machine Learning
- Always cite foundational works (transformers, CNNs, etc.) even if well-known
- Report metrics: accuracy, F1, BLEU, FID, etc. with confidence intervals
- Include computational cost (parameters, FLOPs, training time)
- Reproducibility: dataset splits, random seeds, hyperparameters

### Robotics / ROS2
- Cite hardware platform specs
- Include system latency / real-time performance metrics
- Describe ROS2 node architecture if relevant
- Reference URDF/simulation environment

### General STEM
- Follow domain conventions for methodology section naming
- Check target venue's specific style guide (some IEEE venues have additional constraints)

---

## Reference Files

- `references/ieee-formats.md` — Detailed IEEE format specs and style rules
- `references/latex-snippets.md` — Common LaTeX patterns (algorithms, tables, figures, equations)
