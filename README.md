![logo](https://repository-images.githubusercontent.com/713756230/2e83893d-9aea-43c6-a0b7-3ed4f40da182)

# ⚔️ Saber (Discord Bot)

> Saber is a Discord Bot built with C++ and the [ekizu](https://github.com/jontitorr/ekizu) library.

## Requirements

- Discord Bot Token (**[Guide](https://jontitorr.github.io/ekizu/creating-a-bot-token/)**)
  - Enable 'Message Content Intent' in Discord Developer Portal
- Compiler with C++17 support, i.e. MSVC, GCC, Clang
- [Ekizu](https://github.com/jontitorr/ekizu)
- [CMake](https://cmake.org/) (version >= 3.16)
- [yt-dlp](https://github.com/yt-dlp/yt-dlp)
- [ffmpeg](https://ffmpeg.org/)

## 🚀 Getting Started

```sh
git clone https://www.github.com/jontitorr/saber
cd saber
cmake -S . -B build
cmake --build build --target install --prefix $HOME/.local # or wherever you want
```

If you would like to install a binary instead, check out the [releases](https://github.com/jontitorr/saber/releases).

After installation finishes follow the configuration instructions, then run the bot by running the `saber` executable.

## ⚙️ Configuration

Copy or Rename `config.json.example` to `config.json` and fill out the values:

⚠️ **Note: Never commit or share your token or api keys publicly** ⚠️

```json
{
  "token": "YOUR_TOKEN_HERE",
  "prefix": "!",
  "owner_id": "12345678910",
}
```

## 🤝 Contributing

1. [Fork the repository](https://github.com/jontitorr/saber/fork)
2. Clone your fork: `git clone https://github.com/your-username/saber.git`
3. Create your feature branch: `git checkout -b my-new-feature`
4. Stage changes `git add .`
5. Commit your changes: `git commit`
6. Push to the branch: `git push origin my-new-feature`
7. Submit a pull request
