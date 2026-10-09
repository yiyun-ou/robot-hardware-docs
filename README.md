# SCOUT MINI 机器人硬件资料

本仓库保存 SCOUT MINI 自主移动机器人使用到的硬件文档、手册、SDK 与集成资料。仓库为 Private，仅授权用户可访问。

## 硬件配置

| 类别 | 设备 | 资料入口 |
| --- | --- | --- |
| 移动底盘 | AgileX SCOUT MINI | [机器人](%E6%9C%BA%E5%99%A8%E4%BA%BA/) / [用户手册](SCOUT%20MINI%E7%94%A8%E6%88%B7%E6%89%8B%E5%86%8C.pdf) |
| 主控 | NVIDIA Jetson Orin NX | [Jetson Orin NX资料](Jetson%20Orin%20NX%E8%B5%84%E6%96%99/) |
| 深度相机 | Orbbec Gemini 335 | [奥比中光Gemini335](%E5%A5%A5%E6%AF%94%E4%B8%AD%E5%85%89Gemini335/) |
| 激光雷达 | RoboSense E1R | [E1R激光雷达](E1R%E6%BF%80%E5%85%89%E9%9B%B7%E8%BE%BE/) |
| IMU | WHEELTEC H30WP | [WHEELTEC H30惯导产品资料](WHEELTEC%20H30%E6%83%AF%E5%AF%BC%E4%BA%A7%E5%93%81%E8%B5%84%E6%96%99/) |

## 资料索引与校验

- [完整文件索引](docs/FILE_INDEX.md)
- [Release 大文件索引](docs/LARGE_FILES.md)
- [机器可读清单](manifest.json)
- [上传与远程验证报告](UPLOAD_REPORT.md)
- [Releases](https://github.com/yiyun-ou/robot-hardware-docs/releases)

不超过 50 MiB 的源文件保留原目录结构并由 Git 管理。超过 50 MiB 的文件保存在 GitHub Releases；可用 `python tools/restore_large_files.py` 下载、恢复并校验 SHA-256。

当前清单记录 3,732 个源文件，其中 3,732 个允许上传、0 个因安全审查暂缓、0 个缓存或临时文件忽略。
