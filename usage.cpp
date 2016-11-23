void YourClass::LoadToGLTexture()
{
    // Define in header: HeightGenerator m_hg;
    m_hg.generate(HeightGenerator::GenSeed(), 1024, 0.36f, 14, 0.00055f, "", "height_sample.png");
    
  
    
    int width = m_hg.generatedParam().resolution;
    int height = m_hg.generatedParam().resolution;

    glGenTextures(1, &m_heightMap);
    glBindTexture(GL_TEXTURE_2D, m_heightMap);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0,
        GL_LUMINANCE, GL_UNSIGNED_SHORT, m_hg.generatedData());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

}

// Collision Example:

float Lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

void Terrain::collision(const glm::vec3 &position)
{
    if (position.x > m_terrainOption.horizontalScale || position.x < 0 || 
        position.z > m_terrainOption.horizontalScale || position.z < 0)
        return;

    float xpos = position.x / m_terrainOption.resolutionScale;
    float ypos = position.z / m_terrainOption.resolutionScale;

    double intpart;
    modf(xpos, &intpart);
    float modX = (position.x - intpart * m_terrainOption.resolutionScale) / m_terrainOption.resolutionScale;
    modf(ypos, &intpart);
    float modY = (position.z - intpart * m_terrainOption.resolutionScale) / m_terrainOption.resolutionScale;

    float TopLin = Lerp(getHeightAt((int)xpos, (int)ypos),
        getHeightAt((int)xpos + 1, (int)ypos), modX);
    float BotLin = Lerp(getHeightAt((int)xpos, (int)ypos + 1),
        getHeightAt((int)xpos + 1, (int)ypos + 1), modX);


    ru::Log("height: %.4f", Lerp(TopLin, BotLin, modY) * m_terrainOption.heightScale);
}

float Terrain::getHeightAt(const int x, const int y)
{
    return m_material.m_hg.generatedData()[(y * (int)m_terrainOption.heightMapResolution) + x] / 256.0f;
}
