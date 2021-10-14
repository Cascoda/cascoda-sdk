# PlantUML and Cascoda SDK

Every file in this directory (and subdirectories) with a '.puml' extension will be converted into a .png image using plantUML by running:

``make plantuml``

In order for this to work, the ``CASCODA_PLANTUML_JAR`` cmake cache variable must be set to the path to the plantuml.jar java executable. This can be downloaded from the [PlantUML website](https://plantuml.com).

The generated pngs will be built in-source, so that they can be used in the markdown-generated documentation hosted on Github.
