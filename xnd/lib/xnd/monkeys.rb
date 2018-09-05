class String
  # Force encode string to US-ASCII.
  #
  # FIXME: Hopefully find a better way of doing this soon.
  def b
    self.force_encoding "US-ASCII"
  end

  # Encode string as UTF-16.
  def u16!
    self.encode! Encoding::UTF_16
  end

  # Encode string as UTF-16.
  def u16
    self.encode Encoding::UTF_16
  end

  # Encode string as UTF-32.
  def u32!
    self.encode! Encoding::UTF_32
  end

  # Encode string as UTF-32.
  def u32
    self.encode Encoding::UTF_32
  end
end
