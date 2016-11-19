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
