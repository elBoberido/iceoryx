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

For a local test this command can be used, e.g. for `v3.0.0` on the `release_3.0` branch
```
tools/website/export-docu-to-website.sh local v3.0.0 release_3.0
```
