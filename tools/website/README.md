# Required packages

## System packages

```
apt install doxygen mkdocs pip unzip wget
```

## Python packages

- pip install markdown_include mkdocs-material mkdocs-material-extensions mkdocs-awesome-pages-plugin mkdocs-autolinks-plugin

## Doxybook

Install in docker root

```
wget https://github.com/matusnovak/doxybook2/releases/download/v1.5.0/doxybook2-linux-amd64-v1.5.0.zip
unzip doxybook2-linux-amd64-v1.5.0.zip
```

# Run script

For a local test run this command
```
tools/website/website-generator.sh clean serve
```

For publishing preparation run this command
```
tools/website/website-generator.sh clean --prepare-publish v3.0.0
```
