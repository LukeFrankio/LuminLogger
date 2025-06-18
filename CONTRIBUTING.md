# Contributing to LuminLogger

Thank you for considering contributing to LuminLogger! This document outlines the process for contributing to the project.

## Code of Conduct

By participating in this project, you are expected to uphold our Code of Conduct:

- Use welcoming and inclusive language
- Be respectful of differing viewpoints and experiences
- Gracefully accept constructive criticism
- Focus on what is best for the community
- Show empathy towards other community members

## How Can I Contribute?

### Reporting Bugs

Before submitting a bug report:
- Check the issue tracker to see if the issue has already been reported
- Collect information about the bug (OS, compiler version, minimal reproduction case)

When submitting a bug report:
- Use a clear and descriptive title
- Describe the exact steps to reproduce the problem
- Describe the behavior you observed and what you expected to see
- Include any relevant code snippets or error messages

### Suggesting Enhancements

When suggesting an enhancement:
- Use a clear and descriptive title
- Provide a step-by-step description of the suggested enhancement
- Explain why this enhancement would be useful to most users
- List examples of other projects that have similar features, if applicable

### Pull Requests

1. Fork the repository
2. Create a new branch for your feature or bugfix (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Run the tests to ensure your changes don't break existing functionality
5. Commit your changes (`git commit -m 'Add some amazing feature'`)
6. Push to your branch (`git push origin feature/amazing-feature`)
7. Open a Pull Request

#### Pull Request Guidelines

- Follow the coding style of the project (see [Coding Standards](#coding-standards))
- Include tests for new features or bug fixes
- Update documentation as needed
- Keep your pull request focused on a single topic
- Link any relevant issues in the pull request description

## Coding Standards

- Follow the C++23 standard
- Use the existing code style (4 spaces for indentation)
- Use trailing return types (`auto function() -> ReturnType`)
- Prefer pure functions and immutable data where possible
- Document all public functions and classes with Doxygen-style comments
- Write meaningful commit messages

## Setting Up Development Environment

1. Clone the repository
   ```bash
   git clone https://github.com/yourusername/lumin-logger.git
   cd lumin-logger
   ```

2. Create a build directory
   ```bash
   mkdir build && cd build
   ```

3. Configure with CMake
   ```bash
   cmake ..
   ```

4. Build the project
   ```bash
   cmake --build .
   ```

5. Run tests
   ```bash
   ctest
   ```

## Running Tests

Tests are located in the `tests/` directory. To run all tests:

```bash
cd build
ctest
```

To run a specific test:

```bash
cd build/bin
./logger_test
```

## Documentation

- Document all public APIs using Doxygen-style comments
- Keep the README.md up to date with any changes
- Update examples when adding new features

## Questions?

Feel free to open an issue with your question or contact the maintainers directly. 