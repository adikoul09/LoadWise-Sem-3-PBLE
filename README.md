# LoadWise: Smart Delivery Planner

A menu-driven C program that decides **which packages to load into a delivery vehicle** so the total value carried is as high as possible. It uses the **Fractional Knapsack** problem and the **Greedy Method**.

| | |
|---|---|
| **Subject** | Analysis of Algorithms (AOA) |
| **Experiment** | PBLE 1: Smart Delivery Planning, Fractional Knapsack |
| **Technique** | Greedy Method |
| **Language** | C |

---

## 1. The problem in plain words

A delivery company has a vehicle that can carry only a limited weight. There are several packages, and each one has:

- a **value** (how much profit it brings), and
- a **weight** (how much space it uses).

You can take a package **completely**, or take just **a part of it** if it doesn't fully fit.
**Goal:** carry the most total value without going over the vehicle's capacity.

---

## 2. The idea behind the solution (Greedy Method)

Take the packages that give the **most value for each kilogram** first.

1. **Ratio:** for every package, calculate `value ÷ weight`.
2. **Sort:** arrange the packages from the highest ratio to the lowest.
3. **Load:** go down the sorted list.
   - If the whole package fits, take all of it.
   - If it doesn't fit, take only the part that fills the remaining space, then stop.

### Worked example

Vehicle capacity = **15**

| Package | Value | Weight | Ratio (value ÷ weight) |
|:-------:|:-----:|:------:|:----------------------:|
| 1 | 40 | 5  | 8  |
| 2 | 30 | 10 | 3  |
| 3 | 50 | 5  | 10 |
| 4 | 20 | 4  | 5  |

Sorted by ratio: **3 (10) → 1 (8) → 4 (5) → 2 (3)**

| Step | Package | What happens | Value added | Space left |
|:----:|:-------:|--------------|:-----------:|:----------:|
| 1 | 3 | weight 5 fits, take all | +50 | 10 |
| 2 | 1 | weight 5 fits, take all | +40 | 5 |
| 3 | 4 | weight 4 fits, take all | +20 | 1 |
| 4 | 2 | weight 10 does not fit, take 1/10 of it | +3 | 0 |

**Maximum value = 50 + 40 + 20 + 3 = 113**, using all 15 units of capacity.

---

## 3. Features

**Required by the PBLE**
- Complete 7-option menu that runs until you choose Exit
- Value/weight ratio calculated inside the program
- Stable sort in decreasing ratio order
- Whole packages first, a fraction only when needed
- Step-by-step output, total weight used, maximum value and time complexity
- Input validation and graceful handling of invalid menu choices
- Built-in test case (the example above)

**Extra features**

| Feature | What it does |
|---------|--------------|
| Merge sort option | Choose bubble sort or merge sort, with comparison and move counts. A "compare both" mode shows the difference. |
| 0/1 knapsack comparison | Shows the best result if packages could **not** be split, and how much fractions gain. |
| Package names | Every table shows names such as "Medical Kit" instead of just numbers. |
| Multiple vehicles | Load 1 to 5 vehicles one after another. |
| Save / load data | Store package data in a text file and reload it later. |
| Export report | Save the final selection report to a text file. |
| ASCII load bar | A picture of each vehicle showing which package filled it. |
| Step-by-step mode | Pause after every greedy step, which is useful for demos. |

---

## 4. How to compile and run

You need a C compiler such as `gcc`. No other libraries are required.

```bash
gcc -Wall -Wextra -std=c99 -o loadwise fractional_knapsack_v2.c
./loadwise
```

On Windows with MinGW, run `loadwise.exe` instead.

---

## 5. Quick start (2 minutes)

1. Run the program.
2. Choose **1**, then **2** to load the built-in example.
3. Choose **5** (Find Maximum Value). Answer `n` to the pause question.
4. Choose **6** to see the selected packages and the load bar.
5. Choose **7** to exit.

You can also do the steps one at a time: **3** (ratios), **4** (sort), then **5**.

---

## 6. The menu

| Option | Name | What it does |
|:------:|------|--------------|
| 1 | Enter Package Details | Enter data manually, load the example (1 or 2 vehicles), load from a file, or save to a file. |
| 2 | Display Package Details | Shows the packages and vehicle capacity. |
| 3 | Calculate Value/Weight Ratio | Computes and shows each package's ratio. |
| 4 | Sort Packages by Ratio | Sorts with bubble sort, merge sort, or compares both. |
| 5 | Find Maximum Value | Runs the greedy algorithm and shows every step, the maximum value, the 0/1 comparison and the time complexity. |
| 6 | Display Selected Packages | Shows how much of each package was taken, the vehicle load bars, and offers to save a report. |
| 7 | Exit | Ends the program. |

If you skip a step, options 4 and 5 run the missing earlier steps for you and tell you so.

---

## 7. Sample output (option 5, built-in example)

```
GREEDY SELECTION
Vehicle capacity = 15.00
--------------------------------------------------------------
Package 3 (Medical Kit, ratio 10.0000): weight 5.00 <= remaining -> take FULL. Value +50.00, remaining capacity = 10.00
Package 1 (Documents, ratio 8.0000): weight 5.00 <= remaining -> take FULL. Value +40.00, remaining capacity = 5.00
Package 4 (Electronics, ratio 5.0000): weight 4.00 <= remaining -> take FULL. Value +20.00, remaining capacity = 1.00
Package 2 (Furniture, ratio 3.0000): weight 10.00 > remaining 1.00 -> take FRACTION 0.1000 (1.00/10.00). Value +3.00, remaining capacity = 0.00
--------------------------------------------------------------

Total weight used : 15.00 / 15.00
*** MAXIMUM VALUE OBTAINED = 113.00 ***

COMPARISON WITH 0/1 KNAPSACK (packages cannot be split)
  Best 0/1 value (DP)          : 110.00
  Gain from allowing fractions : 3.00
```

Option 6 then shows the load bar. Each symbol is a package number and `.` means empty space:

```
Load   : [3333333333333111111111111114444444444222] 100% full
Legend : 3=Medical Kit 1=Documents 4=Electronics 2=Furniture ('.' = empty)
```

---

## 8. Data file format

You can write a data file by hand. Lines starting with `#` are ignored.

```
FKDATA
4
2
8 7
Documents,40,5
Furniture,30,10
Medical Kit,50,5
Electronics,20,4
```

| Line | Meaning |
|------|---------|
| `FKDATA` | Header that marks a valid file |
| `4` | Number of packages |
| `2` | Number of vehicles |
| `8 7` | Capacity of each vehicle (one number per vehicle) |
| `name,value,weight` | One line per package |

If a file has a mistake, the program rejects it and keeps your current data safe.

---

## 9. Time complexity

`n` = number of packages, `k` = number of vehicles, `W` = capacity (for the 0/1 comparison).

| Step | Cost |
|------|------|
| Calculate ratios | O(n) |
| Bubble sort | O(n²) |
| Merge sort | O(n log n) |
| Greedy selection scan | O(n + k) |
| **Overall** | **O(n²)** with bubble sort, **O(n log n)** with merge sort |
| 0/1 comparison (optional) | O(n · W) |

**Why:** sorting is the most expensive step. Bubble sort needs about n(n−1)/2 comparisons in nested loops, while merge sort splits the list log n times and merges n items at each level. The ratio calculation and the greedy scan are single passes, so they are cheaper than the sort.

**Why greedy works here:** because a package can be split, taking the best value-per-weight first can never be beaten. This is not true for the 0/1 version, which is why that comparison needs dynamic programming.

---

## 10. Limits and notes

- Up to **50 packages** and **5 vehicles**.
- Values, weights and capacities must be **greater than 0** and at most 1,000,000,000.
- Package names are cut at 23 characters, and commas in names are replaced by spaces.
- With **several vehicles**, a package that doesn't fit in the space left in one vehicle is split: part goes in that vehicle and the rest goes in the next. The maximum value is the same as one vehicle with the combined capacity.
- The **0/1 comparison** runs only for a single vehicle with whole-number weights and a whole-number capacity of at most 10,000. Otherwise it is skipped with a message.
- The sort is **stable**: packages with equal ratios keep their original order.

---

## 11. Project files

| File | Description |
|------|-------------|
| `fractional_knapsack_v2.c` | The full LoadWise program with all extra features |
| `fractional_knapsack.c` | The basic version with only the required PBLE features |
| `README.md` | This file |
