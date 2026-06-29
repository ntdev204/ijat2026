# IEEE Format Reference

## IEEEtran Document Classes

### Conference Paper
```latex
\documentclass[conference]{IEEEtran}
```
- Double column, 10pt font
- Typically 4–8 pages
- No page numbers in submission
- Margins: top 0.75in, bottom 1in, left/right 0.625in

### Journal Article
```latex
\documentclass[journal]{IEEEtran}
```
- Double column, 10pt font
- 8–12 pages typical
- Abstract: 150–250 words
- Include author photos for final submission

### Transactions
```latex
\documentclass[journal]{IEEEtran}  % same class, different venue rules
```
- Stricter review criteria
- Longer (10–14 pages)
- Must include substantial new content beyond any conference version (>30% new)

### Workshop (Short Paper)
```latex
\documentclass[conference]{IEEEtran}
```
- 2–4 pages
- Condensed: Abstract + Intro + Method + Results + Conclusion
- Often work-in-progress

---

## IEEE Style Rules

### Text
- Use "which" for non-restrictive clauses, "that" for restrictive
- Avoid contractions in formal papers
- Spell out acronyms on first use: "Convolutional Neural Network (CNN)"
- Use "Fig." not "Figure" when referencing in text (except at start of sentence)
- "Table I" not "Table 1" (Roman numerals for tables)

### Math
- Variables in italics: $x$, $\theta$
- Vectors bold: $\mathbf{x}$
- Matrices bold caps: $\mathbf{X}$
- Number equations that are referenced: `\begin{equation}...\label{eq:loss}\end{equation}`

### Figures & Tables
- All figures/tables must be referenced in text before they appear
- Captions below figures, above tables
- Figure captions end with period
- Use `\hline` sparingly in tables; prefer `booktabs` (`\toprule`, `\midrule`, `\bottomrule`)

### References
- IEEE style: numbered, ordered by appearance
- Use `IEEEtran` BibTeX style
- Journal names abbreviated per IEEE standard (e.g., "IEEE Trans. Neural Netw. Learn. Syst.")
- Conference names: spell out first time, abbreviate in BibTeX

---

## Common IEEE Abbreviations

| Full Name | Abbreviation |
|---|---|
| IEEE Transactions on Neural Networks and Learning Systems | IEEE Trans. Neural Netw. Learn. Syst. |
| IEEE Transactions on Pattern Analysis and Machine Intelligence | IEEE Trans. Pattern Anal. Mach. Intell. |
| IEEE Transactions on Robotics | IEEE Trans. Robot. |
| IEEE Transactions on Intelligent Transportation Systems | IEEE Trans. Intell. Transp. Syst. |
| International Conference on Robotics and Automation | Proc. IEEE ICRA |
| International Conference on Computer Vision | Proc. IEEE ICCV |
| Conference on Computer Vision and Pattern Recognition | Proc. IEEE/CVF CVPR |

---

## Page Estimation

- ~500 words per column
- 2 columns per page
- → ~1000 words per page (text only)
- Each half-page figure ≈ 500 words equivalent
- References page: ~25–30 refs per page

For a 6-page conference paper:
- Abstract: ~200 words
- Introduction: ~600 words
- Related Work: ~600 words
- Method: ~1200 words
- Experiments: ~1200 words
- Conclusion: ~300 words
- References: ~30 entries
