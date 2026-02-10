# npm 镜像源配置说明

## 自动配置（已完成）

本项目已预配置淘宝 npm 镜像源，加速国内依赖安装。配置包含两个部分：

### 1. .npmrc 文件
位置：`dashboard/frontend/.npmrc`

```
registry=https://registry.npmmirror.com
```

这会让 npm 默认使用淘宝镜像源。

### 2. package.json preinstall 钩子
在 `package.json` 的 `scripts` 中添加了 `preinstall` 钩子：

```json
"scripts": {
  "preinstall": "npm config set registry https://registry.npmmirror.com",
  ...
}
```

这确保在安装依赖前自动设置镜像源。

## 使用方式

直接运行 `npm install` 即可，无需额外配置：

```bash
cd dashboard/frontend
npm install
```

## 验证镜像源

检查当前使用的镜像源：

```bash
npm config get registry
```

应该显示：`https://registry.npmmirror.com/`

## 临时切换回官方源

如果需要临时使用官方源：

```bash
npm install --registry=https://registry.npmjs.org/
```

## 其他镜像源选项

如果淘宝镜像有问题，可以尝试其他国内镜像：

### 腾讯云镜像
```bash
npm config set registry https://mirrors.cloud.tencent.com/npm/
```

### 华为云镜像
```bash
npm config set registry https://mirrors.huaweicloud.com/repository/npm/
```

### 恢复官方源
```bash
npm config set registry https://registry.npmjs.org/
```

## 注意事项

- 淘宝镜像源（npmmirror.com）是原 npm.taobao.org 的新域名
- 镜像源通常会有几分钟到几小时的同步延迟
- 如果遇到特定包找不到的问题，可以临时切换回官方源
