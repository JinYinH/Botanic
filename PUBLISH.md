# 发布到 GitHub / Publishing

这个文件夹已经是一个**可直接开源的仓库**：58 个文件、12.4 MB，不含任何个人信息
（已扫描：无用户名、无本机路径、无邮箱、无 IP）。截图与试听都是插件界面本身。

---

## 方式 A：GitHub Desktop（最省事，推荐）

1. 下载安装 [GitHub Desktop](https://desktop.github.com/)，用你浏览器里已经登录的账号登录；
2. 菜单 **File → Add local repository…**，选择本文件夹（`Botanic-open-source`）；
   如果提示"not a git repository"，点 **create a repository** → 直接 Create；
3. 右上角点 **Publish repository**；
4. 仓库名填 `Botanic`，**取消勾选 "Keep this code private"**，点 Publish。

完成。仓库会出现在 `https://github.com/<你的用户名>/Botanic`。

## 方式 B：网页上传（不装任何软件）

1. 浏览器打开 <https://github.com/new>；
2. Repository name 填 `Botanic`，选 **Public**，**不要**勾选 "Add a README file"，点 Create repository；
3. 在新仓库页面点 **uploading an existing file**；
4. 把本文件夹里的**所有内容**（不是文件夹本身）拖进上传框，等文件列表出现后点 **Commit changes**。

> 注意：Windows 资源管理器默认隐藏 `.gitignore`，上传前在"查看"里勾选"隐藏的项目"再全选。

## 方式 C：命令行（已装 git 的话）

```bash
cd Botanic-open-source
git init -b main
git add .
git -c user.name="Botanic" -c user.email="botanic@users.noreply.github.com" commit -m "Initial commit: Botanic hybrid wavetable synthesizer"
# 先在网页上建好空仓库，再：
git remote add origin https://github.com/<你的用户名>/Botanic.git
git push -u origin main
```

## 建议的仓库设置（发布后 1 分钟）

- **About**：填一句简介 + Topics：`audio-plugin` `vst3` `synthesizer` `juce` `wavetable` `granular`
- **Releases**：把 `Botanic-Windows.zip` 传上去，写版本号 `v1.4`
- 如果你用命令行提交，记得先把 `user.email` 改成你的 GitHub noreply 邮箱
  （GitHub → Settings → Emails → "Keep my email addresses private" 里能看到），
  这样提交记录里不会出现真实邮箱。

## 许可提醒

仓库以 **GPLv3** 发布（因为使用了 JUCE 7 的 GPLv3 授权选项）。
如果以后想闭源卖，需要购买 JUCE 商业授权并替换掉 GPL 条款。
