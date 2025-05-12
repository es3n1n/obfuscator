# Docker Usage for Obfuscator

This project includes Docker support for building and running the obfuscator tool.

## Using the pre-built image

You can pull the pre-built image from GitHub Container Registry:

```bash
docker pull ghcr.io/[username]/obfuscator:latest
```

Replace `[username]` with your GitHub username.

## Running the obfuscator with Docker

Mount your input files to the `/app` directory:

```bash
docker run --rm -v $(pwd):/app ghcr.io/[username]/obfuscator [arguments]
```

Example:

```bash
docker run --rm -v $(pwd):/app ghcr.io/[username]/obfuscator hehe.exe -f main -t TransformName -v SomeName 1337
```

## Building the Docker image locally

If you want to build the Docker image locally:

```bash
docker build -t obfuscator:local .
```

And run it:

```bash
docker run --rm -v $(pwd):/app obfuscator:local [arguments]
```

## Continuous Integration

The repository is configured with GitHub Actions to automatically build and publish Docker images to GitHub Container Registry on pushes to the main branch and when tags are created.

To create a tagged release:

```bash
git tag v1.0.0
git push origin v1.0.0
```

This will trigger a build and publish of `ghcr.io/[username]/obfuscator:v1.0.0` and `ghcr.io/[username]/obfuscator:1.0`. 