# Issue Tracker

Log of all platform issues detected during testing or work sessions.

## Open Issues
| ID | Date | Priority | Issue Description | Status | Resolution |
|---|---|---|---|---|---|
| 006 | 2024-05-26 | **MEDIUM** | PWS Weather API is currently commented out and non-functional. | **Open** | Needs fixing across multiple files to restore functionality. |
| 007 | 2024-05-26 | **MEDIUM** | Missing support for SD card storage to write logs as an alternative to LittleFS. | **Open** | Re-add SD card driver support and logging logic. |

## Resolved Issues
| ID | Date | Priority | Issue Description | Status | Resolution |
|---|---|---|---|---|---|
| 001 | 2026-05-11 | **CRITICAL** | Mini-charts Y-axis bounds collapse to zero due to `null` placeholder evaluation, and secondary math constraints forced a 5% fixed padding that visually flattened small fluctuations. | **Resolved** | Updated defaults to `undefined`, sanitized filters, and radically decreased fixed padding constraint from 5% to 0.5%, achieving a 10x increase in micro-fluctuation visual resolution. |
| 002 | 2026-05-11 | **LOW** | Battery historical chart was constrained to 1 decimal place, masking small fluctuations. | **Resolved** | Refactored `createHistoricalSVGChart` for parametric decimal control and forced battery render to 2 decimal places. |
| 003 | 2026-05-16 | **LOW** | Sidebar Restart link contained stray HTML tags and inconsistent icon sizing. | **Resolved** | Cleaned up HTML structure and standardized SVG icons for the new sidebar toggle implementation. |
| 004 | 2026-05-11 | **MEDIUM** | Daily Weather Records table requires formatting and logic fixes. | **Resolved** | Table has been fixed and verified. |
| 005 | 2026-05-16 | **LOW** | 24-hour Wind Speed Graph lacks direction indicators for historical analysis. | **Resolved** | Direction indicators added to the SVG chart logic. |
