# GitHub Actions Documentation

## Build volmix Debian Package Action

This repository includes a GitHub Action that builds Debian packages using our Docker-based build system.

### Manual Trigger

The action can be triggered manually from the GitHub web interface:

1. **Navigate to Actions Tab**
   - Go to your repository on GitHub
   - Click the "Actions" tab
   - Select "Build volmix Debian Package" workflow

2. **Run Workflow**
   - Click "Run workflow" button
   - Choose your options:
     - **Architecture**: `amd64` or `arm64` (default: `amd64`)
     - **Build Type**: `release` or `debug` (default: `release`) 
     - **Retention Days**: How long to keep artifacts (default: `30`)

3. **Monitor Progress**
   - Watch the build progress in real-time
   - View detailed logs for each step
   - See build summary with package details

### Downloading Artifacts

After a successful build:

1. **From GitHub Web Interface**
   - Go to the completed workflow run
   - Scroll to "Artifacts" section
   - Click to download `volmix-packages-{arch}-{type}.zip`

2. **Using GitHub CLI**
   ```bash
   # List recent runs
   gh run list --workflow="Build volmix Debian Package"
   
   # Download artifacts from specific run
   gh run download <run-id>
   ```

3. **Extract and Install**
   ```bash
   # Extract the zip file
   unzip volmix-packages-amd64-release.zip
   
   # Install the package
   sudo dpkg -i volmix_*.deb
   ```

### Build Features

- **Docker-based**: Uses our existing `Dockerfile` and `build-package.sh`
- **Reproducible**: Clean build environment every time
- **Multi-architecture**: Support for amd64 and arm64 (future)
- **Artifact Storage**: Packages stored as GitHub Actions artifacts
- **Build Summary**: Detailed information about generated packages
- **Error Handling**: Clear error messages and validation

### Workflow Details

The workflow performs these steps:

1. **Setup**: Checkout code and configure Docker Buildx
2. **Build**: Execute our build script in Docker container
3. **Verify**: Check that .deb packages were created successfully
4. **Upload**: Store packages as artifacts with retention policy
5. **Summary**: Generate build report with package information

### Future Enhancements

Planned improvements:

- **Automatic Releases**: Trigger on version tags to create GitHub releases
- **Multi-architecture**: Native arm64 builds
- **Testing**: Automated package installation testing
- **Security**: Package signing and vulnerability scanning
- **Performance**: Build caching for faster subsequent builds

### Troubleshooting

Common issues and solutions:

- **"No .deb packages found"**: Check Docker build logs for compilation errors
- **Permission errors**: Ensure `build-package.sh` has execute permissions
- **Artifact download fails**: Check retention period hasn't expired
- **Build timeout**: Large packages may need longer timeout limits

For more details, see the workflow file: `.github/workflows/build-package.yml`