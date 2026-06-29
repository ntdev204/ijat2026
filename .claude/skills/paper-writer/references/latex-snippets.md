# LaTeX Snippets for IEEE Papers

## Algorithm Block
```latex
\usepackage{algorithm}
\usepackage{algorithmic}

\begin{algorithm}
\caption{Algorithm Name}
\label{alg:myalg}
\begin{algorithmic}[1]
\REQUIRE Input $x$, parameter $\theta$
\ENSURE Output $y$
\STATE Initialize $y \leftarrow 0$
\FOR{$i = 1$ \TO $N$}
  \STATE $y \leftarrow y + f(x_i, \theta)$
\ENDFOR
\RETURN $y$
\end{algorithmic}
\end{algorithm}
```

## Table (with booktabs)
```latex
\usepackage{booktabs}

\begin{table}[t]
\caption{Comparison of Methods}
\label{tab:results}
\centering
\begin{tabular}{lccc}
\toprule
Method & Accuracy & F1 & Params \\
\midrule
Baseline & 82.3 & 81.1 & 12M \\
Ours & \textbf{87.6} & \textbf{86.4} & 14M \\
\bottomrule
\end{tabular}
\end{table}
```

## Figure
```latex
\begin{figure}[t]
\centering
\includegraphics[width=\columnwidth]{figures/architecture.pdf}
\caption{Overview of the proposed architecture. (a) encoder, (b) decoder.}
\label{fig:arch}
\end{figure}
```

## Double-column Figure
```latex
\begin{figure*}[t]
\centering
\includegraphics[width=\textwidth]{figures/results.pdf}
\caption{Qualitative results across all datasets.}
\label{fig:results}
\end{figure*}
```

## Equation
```latex
\begin{equation}
\mathcal{L} = \mathcal{L}_{ce} + \lambda \mathcal{L}_{reg}
\label{eq:loss}
\end{equation}
```

## Aligned Equations
```latex
\begin{align}
\hat{y} &= \sigma(\mathbf{W}\mathbf{x} + \mathbf{b}) \label{eq:pred} \\
\mathcal{L} &= -\sum_{i} y_i \log \hat{y}_i \label{eq:loss}
\end{align}
```

## Subfigures
```latex
\usepackage{subfig}

\begin{figure}[t]
\centering
\subfloat[Caption A]{\includegraphics[width=0.48\columnwidth]{fig_a.pdf}\label{fig:a}}
\hfill
\subfloat[Caption B]{\includegraphics[width=0.48\columnwidth]{fig_b.pdf}\label{fig:b}}
\caption{Overall caption for both subfigures.}
\label{fig:both}
\end{figure}
```

## Code Listing
```latex
\usepackage{listings}
\usepackage{xcolor}

\lstset{
  basicstyle=\ttfamily\footnotesize,
  keywordstyle=\color{blue},
  commentstyle=\color{gray},
  stringstyle=\color{red},
  breaklines=true,
  frame=single
}

\begin{lstlisting}[language=Python, caption={ROS2 node example}]
import rclpy
from rclpy.node import Node

class MyNode(Node):
    def __init__(self):
        super().__init__('my_node')
\end{lstlisting}
```

## Theorem / Definition
```latex
\usepackage{amsthm}
\newtheorem{theorem}{Theorem}
\newtheorem{definition}{Definition}

\begin{definition}
A \emph{graph} $G = (V, E)$ consists of...
\end{definition}

\begin{theorem}
For any graph $G$, the following holds...
\end{theorem}
\begin{proof}
...
\end{proof}
```

## Multi-column Table
```latex
\begin{table*}[t]
\caption{Extended Results Across All Benchmarks}
\label{tab:full}
\centering
\begin{tabular}{lcccccc}
\toprule
 & \multicolumn{2}{c}{Dataset A} & \multicolumn{2}{c}{Dataset B} & \multicolumn{2}{c}{Dataset C} \\
\cmidrule(lr){2-3} \cmidrule(lr){4-5} \cmidrule(lr){6-7}
Method & Acc & F1 & Acc & F1 & Acc & F1 \\
\midrule
Baseline & 80.1 & 79.3 & 75.2 & 74.1 & 88.3 & 87.1 \\
Ours & \textbf{85.4} & \textbf{84.7} & \textbf{81.0} & \textbf{80.2} & \textbf{91.2} & \textbf{90.5} \\
\bottomrule
\end{tabular}
\end{table*}
```

## BibTeX Entry Templates

### Conference paper
```bibtex
@inproceedings{author2024keyword,
  author    = {Last, First and Last2, First2},
  title     = {Paper Title},
  booktitle = {Proceedings of the IEEE/CVF Conference on Computer Vision and Pattern Recognition (CVPR)},
  year      = {2024},
  pages     = {1234--1243},
  doi       = {10.1109/CVPR52733.2024.00123}
}
```

### Journal article
```bibtex
@article{author2023keyword,
  author  = {Last, First and Last2, First2},
  title   = {Article Title},
  journal = {IEEE Transactions on Pattern Analysis and Machine Intelligence},
  year    = {2023},
  volume  = {45},
  number  = {8},
  pages   = {9876--9891},
  doi     = {10.1109/TPAMI.2023.3012345}
}
```

### arXiv preprint
```bibtex
@article{author2024arxiv,
  author  = {Last, First},
  title   = {Preprint Title},
  journal = {arXiv preprint arXiv:2401.12345},
  year    = {2024},
  url     = {https://arxiv.org/abs/2401.12345}
}
```

### Book chapter
```bibtex
@incollection{author2022chapter,
  author    = {Last, First},
  title     = {Chapter Title},
  booktitle = {Book Title},
  publisher = {Springer},
  year      = {2022},
  pages     = {123--145}
}
```
